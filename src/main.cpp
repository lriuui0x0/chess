#include "../lib/util.hpp"
#include "../lib/vulkan.hpp"
#include "../lib/os.hpp"
#include "math.cpp"
#include "asset.cpp"
#include "window.cpp"
#include "scene.cpp"
#include "shadow.cpp"
#include "debug_ui.cpp"
#include "debug_collision.cpp"

Bool read_mesh(CStr filename, Mesh *mesh)
{
    Str file_contents;
    if (!read_file(filename, &file_contents))
    {
        return false;
    }

    if (!deserialise_mesh(file_contents, mesh))
    {
        return false;
    }

    return true;
}

Bool read_image(CStr filename, Image *image)
{
    Str file_contents;
    if (!read_file(filename, &file_contents))
    {
        return false;
    }

    if (!deserialise_image(file_contents, image))
    {
        return false;
    }

    return true;
}

Bool read_font(CStr filename, Font *font)
{
    Str file_contents;
    if (!read_file(filename, &file_contents))
    {
        return false;
    }

    if (!deserialise_font(file_contents, font))
    {
        return false;
    }

    return true;
}

Bool read_bit_board_table(CStr filename, BitBoardTable *bit_board_table)
{
    Str file_contents;
    if (!read_file(filename, &file_contents))
    {
        return false;
    }

    if (!deserialise_bit_board_table(file_contents, bit_board_table))
    {
        return false;
    }

    return true;
}

Void write_entity_vertex_data(Entity *entity, VulkanBuffer *scene_vertex_buffer, VulkanBuffer *scene_index_buffer)
{
    Mesh *mesh = entity->mesh;
    Int vertex_data_length = mesh->vertex_count * sizeof(mesh->vertices_data[0]);
    Int index_data_length = mesh->index_count * sizeof(mesh->indices_data[0]);
    memcpy(scene_vertex_buffer->data + mesh->vertex_offset * sizeof(mesh->vertices_data[0]), mesh->vertices_data, vertex_data_length);
    memcpy(scene_index_buffer->data + mesh->index_offset * sizeof(mesh->indices_data[0]), mesh->indices_data, index_data_length);
}

Void debug_callback(Str message)
{
    OutputDebugStringA((LPCSTR)message.data);
    OutputDebugStringA("\n");
}

Bool render_vulkan_frame(VulkanDevice *device,
                         VulkanPipeline *scene_pipeline, SceneFrame *scene_frame, VulkanBuffer *scene_uniform_buffer,
                         Board *board, Piece *pieces, GhostPiece *ghost_piece,
                         VulkanPipeline *shadow_pipeline, ShadowFrame *shadow_frame,
                         VulkanPipeline *debug_ui_pipeline, DebugUIFrame *debug_ui_frame, VulkanBuffer *debug_ui_vertex_buffer, Int debug_ui_character_count,
                         VulkanPipeline *debug_collision_pipeline, DebugCollisionFrame *debug_collision_frame, VulkanBuffer *debug_collision_vertex_buffer)
{
    VkResult result_code;
    result_code = vkWaitForFences(device->handle, 1, &scene_frame->frame_finished_fence, false, UINT64_MAX);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    result_code = vkResetFences(device->handle, 1, &scene_frame->frame_finished_fence);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    Int image_index;
    result_code = vkAcquireNextImageKHR(device->handle, device->swapchain.handle, UINT64_MAX, scene_frame->image_aquired_semaphore, VK_NULL_HANDLE, (UInt32 *)&image_index);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferBeginInfo command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result_code = vkBeginCommandBuffer(scene_frame->command_buffer, &command_buffer_begin_info);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkBufferCopy scene_uniform_buffer_copy = {};
    scene_uniform_buffer_copy.srcOffset = 0;
    scene_uniform_buffer_copy.dstOffset = 0;
    scene_uniform_buffer_copy.size = scene_uniform_buffer->count;
    vkCmdCopyBuffer(scene_frame->command_buffer, scene_uniform_buffer->handle, scene_frame->uniform_buffer.handle, 1, &scene_uniform_buffer_copy);

    VkBufferMemoryBarrier scene_uniform_buffer_memory_barrier = {};
    scene_uniform_buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    scene_uniform_buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    scene_uniform_buffer_memory_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
    scene_uniform_buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    scene_uniform_buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    scene_uniform_buffer_memory_barrier.buffer = scene_frame->uniform_buffer.handle;
    scene_uniform_buffer_memory_barrier.offset = 0;
    scene_uniform_buffer_memory_barrier.size = scene_uniform_buffer->count;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, null, 1, &scene_uniform_buffer_memory_barrier, 0, null);

    VkImageMemoryBarrier depth_image_memory_barrier = {};
    depth_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    depth_image_memory_barrier.srcAccessMask = 0;
    depth_image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depth_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depth_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depth_image_memory_barrier.image = scene_frame->depth_image;
    depth_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    depth_image_memory_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    depth_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    depth_image_memory_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, null, 0, null, 1, &depth_image_memory_barrier);

    VkImageMemoryBarrier color_image_memory_barrier = {};
    color_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    color_image_memory_barrier.srcAccessMask = 0;
    color_image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    color_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    color_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    color_image_memory_barrier.image = scene_frame->color_image;
    color_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    color_image_memory_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    color_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    color_image_memory_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, null, 0, null, 1, &color_image_memory_barrier);

    // NOTE: Shadow
    VkClearValue shadow_clear_colors[1] = {{1.0, 0}};
    VkRenderPassBeginInfo shadow_render_pass_begin_info = {};
    shadow_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    shadow_render_pass_begin_info.renderPass = shadow_pipeline->render_pass;
    shadow_render_pass_begin_info.framebuffer = shadow_frame->frame_buffers[image_index];
    shadow_render_pass_begin_info.renderArea.offset = {0, 0};
    shadow_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
    shadow_render_pass_begin_info.clearValueCount = 1;
    shadow_render_pass_begin_info.pClearValues = shadow_clear_colors;

    vkCmdBeginRenderPass(scene_frame->command_buffer, &shadow_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline->handle);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &scene_frame->vertex_buffer.handle, &offset);
    vkCmdBindIndexBuffer(scene_frame->command_buffer, scene_frame->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline->layout, 0, 1, &scene_frame->scene_descriptor_set, 0, null);

    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline->layout, 1, 1, &scene_frame->board_descriptor_set, 0, null);
    vkCmdDrawIndexed(scene_frame->command_buffer, board->mesh->index_count, 1, board->mesh->index_offset, board->mesh->vertex_offset, 0);

    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        Mesh *mesh = pieces[piece_i].mesh;
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline->layout, 1, 1, &scene_frame->piece_descriptor_sets[piece_i], 0, null);
        vkCmdDrawIndexed(scene_frame->command_buffer, mesh->index_count, 1, mesh->index_offset, mesh->vertex_offset, 0);
    }

    vkCmdEndRenderPass(scene_frame->command_buffer);

    VkImageMemoryBarrier shadow_depth_image_memory_barrier = {};
    shadow_depth_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    shadow_depth_image_memory_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadow_depth_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    shadow_depth_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    shadow_depth_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    shadow_depth_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadow_depth_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadow_depth_image_memory_barrier.image = shadow_frame->depth_image;
    shadow_depth_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    shadow_depth_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    shadow_depth_image_memory_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    shadow_depth_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    shadow_depth_image_memory_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, null, 0, null, 1, &shadow_depth_image_memory_barrier);

    // NOTE: Scene
    VkClearValue scene_clear_colors[2] = {{0.7, 0.7, 0.7, 0.7},
                                          {1.0, 0}};
    VkRenderPassBeginInfo scene_render_pass_begin_info = {};
    scene_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    scene_render_pass_begin_info.renderPass = scene_pipeline->render_pass;
    scene_render_pass_begin_info.framebuffer = scene_frame->frame_buffers[image_index];
    scene_render_pass_begin_info.renderArea.offset = {0, 0};
    scene_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
    scene_render_pass_begin_info.clearValueCount = 2;
    scene_render_pass_begin_info.pClearValues = scene_clear_colors;

    vkCmdBeginRenderPass(scene_frame->command_buffer, &scene_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->handle);

    offset = 0;
    vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &scene_frame->vertex_buffer.handle, &offset);
    vkCmdBindIndexBuffer(scene_frame->command_buffer, scene_frame->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 0, 1, &scene_frame->scene_descriptor_set, 0, null);
    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 3, 1, &scene_frame->shadow_descriptor_set, 0, null);

    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 1, 1, &scene_frame->board_descriptor_set, 0, null);
    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 2, 1, &scene_frame->lightmap_descriptor_sets[board->lightmap_index], 0, null);
    vkCmdDrawIndexed(scene_frame->command_buffer, board->mesh->index_count, 1, board->mesh->index_offset, board->mesh->vertex_offset, 0);

    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        if (ghost_piece == null || piece_i != ghost_piece->shadowed_piece_index)
        {
            Mesh *mesh = pieces[piece_i].mesh;
            Int lightmap_index = pieces[piece_i].lightmap_index;
            vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 1, 1, &scene_frame->piece_descriptor_sets[piece_i], 0, null);
            vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 2, 1, &scene_frame->lightmap_descriptor_sets[lightmap_index], 0, null);
            vkCmdDrawIndexed(scene_frame->command_buffer, mesh->index_count, 1, mesh->index_offset, mesh->vertex_offset, 0);
        }
    }

    if (ghost_piece)
    {
        Mesh *mesh = ghost_piece->mesh;
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 1, 1, &scene_frame->ghost_piece_descriptor_set, 0, null);
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 2, 1, &scene_frame->lightmap_descriptor_sets[ghost_piece->lightmap_index], 0, null);
        vkCmdDrawIndexed(scene_frame->command_buffer, mesh->index_count, 1, mesh->index_offset, mesh->vertex_offset, 0);
    }

    vkCmdEndRenderPass(scene_frame->command_buffer);

    // NOTE: Debug collision
    if (debug_ui_character_count > 0)
    {
        VkRenderPassBeginInfo debug_collision_render_pass_begin_info = {};
        debug_collision_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        debug_collision_render_pass_begin_info.renderPass = debug_collision_pipeline->render_pass;
        debug_collision_render_pass_begin_info.framebuffer = debug_collision_frame->frame_buffers[image_index];
        debug_collision_render_pass_begin_info.renderArea.offset = {0, 0};
        debug_collision_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
        debug_collision_render_pass_begin_info.clearValueCount = 0;
        debug_collision_render_pass_begin_info.pClearValues = null;

        vkCmdBeginRenderPass(scene_frame->command_buffer, &debug_collision_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_collision_pipeline->handle);

        offset = 0;
        vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &debug_collision_frame->vertex_buffer.handle, &offset);

        Int vertex_offset = 0;
        for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
        {
            Mesh *mesh = pieces[piece_i].mesh;
            Int vertex_count = COLLISION_BOX_VERTEX_COUNT + mesh->collision_hull_vertex_count * 2;

            vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_collision_pipeline->layout, 1, 1, &scene_frame->piece_descriptor_sets[piece_i], 0, null);
            vkCmdDraw(scene_frame->command_buffer, vertex_count, 1, vertex_offset, 0);
            vertex_offset += vertex_count;
        }

        vkCmdEndRenderPass(scene_frame->command_buffer);
    }

    // NOTE: Debug UI
    if (debug_ui_character_count > 0)
    {
        VkBufferCopy debug_ui_vertex_buffer_copy = {};
        debug_ui_vertex_buffer_copy.srcOffset = 0;
        debug_ui_vertex_buffer_copy.dstOffset = 0;
        debug_ui_vertex_buffer_copy.size = sizeof(DebugUIVertex) * debug_ui_character_count * 6;
        vkCmdCopyBuffer(scene_frame->command_buffer, debug_ui_vertex_buffer->handle, debug_ui_frame->vertex_buffer.handle, 1, &debug_ui_vertex_buffer_copy);

        VkBufferMemoryBarrier debug_ui_vertex_buffer_memory_barrier = {};
        debug_ui_vertex_buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        debug_ui_vertex_buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        debug_ui_vertex_buffer_memory_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        debug_ui_vertex_buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        debug_ui_vertex_buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        debug_ui_vertex_buffer_memory_barrier.buffer = debug_ui_frame->vertex_buffer.handle;
        debug_ui_vertex_buffer_memory_barrier.offset = 0;
        debug_ui_vertex_buffer_memory_barrier.size = sizeof(DebugUIVertex) * debug_ui_character_count * 6;
        vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, null, 1, &debug_ui_vertex_buffer_memory_barrier, 0, null);

        VkRenderPassBeginInfo debug_ui_render_pass_begin_info = {};
        debug_ui_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        debug_ui_render_pass_begin_info.renderPass = debug_ui_pipeline->render_pass;
        debug_ui_render_pass_begin_info.framebuffer = debug_ui_frame->frame_buffers[image_index];
        debug_ui_render_pass_begin_info.renderArea.offset = {0, 0};
        debug_ui_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
        debug_ui_render_pass_begin_info.clearValueCount = 0;
        debug_ui_render_pass_begin_info.pClearValues = null;

        vkCmdBeginRenderPass(scene_frame->command_buffer, &debug_ui_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_ui_pipeline->handle);

        offset = 0;
        vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &debug_ui_frame->vertex_buffer.handle, &offset);

        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_ui_pipeline->layout, 0, 1, &debug_ui_frame->font_texture_descriptor_set, 0, null);

        vkCmdDraw(scene_frame->command_buffer, debug_ui_character_count * 2 * 3, 1, 0, 0);

        vkCmdEndRenderPass(scene_frame->command_buffer);
    }

    result_code = vkEndCommandBuffer(scene_frame->command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &scene_frame->image_aquired_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &scene_frame->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &scene_frame->render_finished_semaphore;

    result_code = vkQueueSubmit(device->graphics_queue, 1, &submit_info, scene_frame->frame_finished_fence);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &scene_frame->render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &device->swapchain.handle;
    present_info.pImageIndices = (UInt32 *)&image_index,
    present_info.pResults = null;

    result_code = vkQueuePresentKHR(device->present_queue, &present_info);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

struct Input
{
    Int mouse_x;
    Int mouse_y;
    Bool click_left;
    Bool click_right;

    Bool keydown_g;
    Bool keydown_z;

    Bool keydown_d;
    Bool keyup_d;
    Bool keydown_a;
    Bool keyup_a;
    Bool keydown_q;
    Bool keyup_q;
    Bool keydown_e;
    Bool keyup_e;
    Bool keydown_w;
    Bool keyup_w;
    Bool keydown_s;
    Bool keyup_s;

    Bool keydown_i;
    Bool keyup_i;
    Bool keydown_k;
    Bool keyup_k;
    Bool keydown_l;
    Bool keyup_l;
    Bool keydown_j;
    Bool keyup_j;
    Bool keydown_o;
    Bool keyup_o;
    Bool keydown_u;
    Bool keyup_u;
};

struct DebugState
{
    Bool show_debug_ui;
    Bool moving_x_pos;
    Bool moving_x_neg;
    Bool moving_y_pos;
    Bool moving_y_neg;
    Bool moving_z_pos;
    Bool moving_z_neg;
    Bool rotating_x_pos;
    Bool rotating_x_neg;
    Bool rotating_y_pos;
    Bool rotating_y_neg;
    Bool rotating_z_pos;
    Bool rotating_z_neg;
};

struct State
{
    Int mouse_x;
    Int mouse_y;

    DebugState debug;
};

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    RandomGenerator random_generator;
    random_generator.seed = get_current_timestamp();

    Mesh board_mesh;
    ASSERT(read_mesh("../asset/board.asset", &board_mesh));

    Mesh black_rook_mesh;
    ASSERT(read_mesh("../asset/rook_black.asset", &black_rook_mesh));
    Mesh black_knight_mesh;
    ASSERT(read_mesh("../asset/knight_black.asset", &black_knight_mesh));
    Mesh black_bishop_mesh;
    ASSERT(read_mesh("../asset/bishop_black.asset", &black_bishop_mesh));
    Mesh black_queen_mesh;
    ASSERT(read_mesh("../asset/queen_black.asset", &black_queen_mesh));
    Mesh black_king_mesh;
    ASSERT(read_mesh("../asset/king_black.asset", &black_king_mesh));
    Mesh black_pawn_mesh;
    ASSERT(read_mesh("../asset/pawn_black.asset", &black_pawn_mesh));

    Mesh white_rook_mesh;
    ASSERT(read_mesh("../asset/rook_white.asset", &white_rook_mesh));
    Mesh white_knight_mesh;
    ASSERT(read_mesh("../asset/knight_white.asset", &white_knight_mesh));
    Mesh white_bishop_mesh;
    ASSERT(read_mesh("../asset/bishop_white.asset", &white_bishop_mesh));
    Mesh white_queen_mesh;
    ASSERT(read_mesh("../asset/queen_white.asset", &white_queen_mesh));
    Mesh white_king_mesh;
    ASSERT(read_mesh("../asset/king_white.asset", &white_king_mesh));
    Mesh white_pawn_mesh;
    ASSERT(read_mesh("../asset/pawn_white.asset", &white_pawn_mesh));

    Array<Image> lightmaps = create_array<Image>();
    Image *board_lightmap = lightmaps.push();
    ASSERT(read_image("../asset/board_lightmap.asset", board_lightmap));
    Image *rook_lightmap = lightmaps.push();
    ASSERT(read_image("../asset/rook_lightmap.asset", rook_lightmap));
    Image *knight_lightmap = lightmaps.push();
    ASSERT(read_image("../asset/knight_lightmap.asset", knight_lightmap));
    Image *bishop_lightmap = lightmaps.push();
    ASSERT(read_image("../asset/bishop_lightmap.asset", bishop_lightmap));
    Image *queen_lightmap = lightmaps.push();
    ASSERT(read_image("../asset/queen_lightmap.asset", queen_lightmap));
    Image *king_lightmap = lightmaps.push();
    ASSERT(read_image("../asset/king_lightmap.asset", king_lightmap));
    Image *pawn_lightmap = lightmaps.push();
    ASSERT(read_image("../asset/pawn_lightmap.asset", pawn_lightmap));

    Font debug_font;
    ASSERT(read_font("../asset/debug_font.asset", &debug_font));

    BitBoardTable bit_board_table;
    ASSERT(read_bit_board_table("../asset/bitboard.asset", &bit_board_table));

    GameState game_state = get_initial_game_state();

    Board board;
    fill_board_initial_state(&board, &board_mesh, 0);

    Array<Piece> pieces = create_array<Piece>(32);
    for (Int square = 0; square < 64; square++)
    {
        Piece *piece = pieces.push();
        GamePiece game_piece = game_state.position.board[square];
        if (game_piece)
        {
            fill_piece_initial_state(game_piece, piece, square,
                                     &white_rook_mesh,
                                     &white_knight_mesh,
                                     &white_bishop_mesh,
                                     &white_queen_mesh,
                                     &white_king_mesh,
                                     &white_pawn_mesh,
                                     &black_rook_mesh,
                                     &black_knight_mesh,
                                     &black_bishop_mesh,
                                     &black_queen_mesh,
                                     &black_king_mesh,
                                     &black_pawn_mesh,
                                     1, 2, 3, 4, 5, 6);
        }
    }

    GhostPiece ghost_piece;
    fill_ghost_piece_initila_state(&ghost_piece);

    Int window_width = 800;
    Int window_height = 600;
    Window window = create_window(str("Chess"), window_width, window_height, 50, 50);
    ASSERT(window);

    VulkanDevice device;
    ASSERT(create_device(window, debug_callback, &device));
    ASSERT(create_swapchain(&device, window_width, window_height, 3, VK_PRESENT_MODE_MAILBOX_KHR));

    VulkanPipeline scene_pipeline;
    ASSERT(create_scene_pipeline(&device, &scene_pipeline));

    VulkanPipeline shadow_pipeline;
    ASSERT(create_shadow_pipeline(&device, &shadow_pipeline));

    VulkanPipeline debug_ui_pipeline;
    ASSERT(create_debug_ui_pipeline(&device, &debug_ui_pipeline));

    VulkanPipeline debug_collision_pipeline;
    ASSERT(create_debug_collision_pipeline(&device, &debug_collision_pipeline));

    ShadowFrame shadow_frame;
    ASSERT(create_shadow_frame(&device, &shadow_pipeline, &shadow_frame));

    SceneFrame scene_frame;
    VulkanBuffer scene_vertex_buffer;
    VulkanBuffer scene_index_buffer;
    VulkanBuffer scene_uniform_buffer;
    ASSERT(create_scene_frame(&device, &scene_pipeline, &board, &pieces, &lightmaps, &shadow_frame, &scene_frame, &scene_vertex_buffer, &scene_index_buffer, &scene_uniform_buffer));

    DebugUIFrame debug_ui_frame;
    VulkanBuffer debug_ui_vertex_buffer;
    ASSERT(create_debug_ui_frame(&device, &debug_ui_pipeline, &debug_font, &debug_ui_frame, &debug_ui_vertex_buffer));

    DebugCollisionFrame debug_collision_frame;
    VulkanBuffer debug_collision_vertex_buffer;
    ASSERT(create_debug_collision_frame(&device, &debug_collision_pipeline, pieces, &debug_collision_frame, &debug_collision_vertex_buffer));

    Camera camera = get_scene_camera();

    SceneUniformData *scene_uniform_data = get_scene_uniform_data(&device, &scene_uniform_buffer);
    calculate_scene_uniform_data(&camera, window_width, window_height, scene_uniform_data);

    calculate_entity_uniform_data(&board, get_board_uniform_data(&device, &scene_uniform_buffer));
    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        Piece *piece = &pieces[piece_i];
        calculate_entity_uniform_data(piece, get_piece_uniform_data(&device, &scene_uniform_buffer, piece_i));
    }

    write_entity_vertex_data(&board, &scene_vertex_buffer, &scene_index_buffer);
    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        write_entity_vertex_data(&pieces[piece_i], &scene_vertex_buffer, &scene_index_buffer);
    }
    ASSERT(upload_buffer(&device, &scene_vertex_buffer, &scene_frame.vertex_buffer));
    ASSERT(upload_buffer(&device, &scene_index_buffer, &scene_frame.index_buffer));

    DebugCollisionVertex *vertex = (DebugCollisionVertex *)debug_collision_vertex_buffer.data;
    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        vertex = write_collision_data(pieces[piece_i].mesh, vertex);
    }
    ASSERT(upload_buffer(&device, &debug_collision_vertex_buffer, &debug_collision_frame.vertex_buffer));

    State state = {};

    show_window(window);

    Bool is_running = true;

    Int last_hover_row = -1;
    Int last_hover_column = -1;

    UInt64 last_timestamp = get_current_timestamp();
    Real frame_time = 1.0 / 60.0;
    while (is_running)
    {
        WindowMessage message;
        Input input = {};
        input.mouse_x = -1;
        input.mouse_y = -1;
        while (get_window_message(window, &message))
        {
            switch (message.type)
            {
            case WindowMessageType::close:
            {
                is_running = false;
            }
            break;

            case WindowMessageType::key_down:
            {
                WindowMessageKeyCode key_code = message.key_down_data.key_code;

                if (key_code == WindowMessageKeyCode::key_d)
                {
                    input.keyup_d = true;
                    input.keydown_d = true;
                }
                else if (key_code == WindowMessageKeyCode::key_a)
                {
                    input.keyup_a = false;
                    input.keydown_a = true;
                }
                else if (key_code == WindowMessageKeyCode::key_q)
                {
                    input.keyup_q = false;
                    input.keydown_q = true;
                }
                else if (key_code == WindowMessageKeyCode::key_e)
                {
                    input.keyup_e = false;
                    input.keydown_e = true;
                }
                else if (key_code == WindowMessageKeyCode::key_w)
                {
                    input.keyup_w = false;
                    input.keydown_w = true;
                }
                else if (key_code == WindowMessageKeyCode::key_s)
                {
                    input.keyup_s = false;
                    input.keydown_s = true;
                }
                else if (key_code == WindowMessageKeyCode::key_i)
                {
                    input.keyup_i = false;
                    input.keydown_i = true;
                }
                else if (key_code == WindowMessageKeyCode::key_k)
                {
                    input.keyup_k = false;
                    input.keydown_k = true;
                }
                else if (key_code == WindowMessageKeyCode::key_l)
                {
                    input.keyup_l = false;
                    input.keydown_l = true;
                }
                else if (key_code == WindowMessageKeyCode::key_j)
                {
                    input.keyup_j = false;
                    input.keydown_j = true;
                }
                else if (key_code == WindowMessageKeyCode::key_o)
                {
                    input.keyup_o = false;
                    input.keydown_o = true;
                }
                else if (key_code == WindowMessageKeyCode::key_u)
                {
                    input.keyup_u = false;
                    input.keydown_u = true;
                }
                else if (key_code == WindowMessageKeyCode::key_g)
                {
                    input.keydown_g = true;
                }
                else if (key_code == WindowMessageKeyCode::key_z)
                {
                    input.keydown_z = true;
                }
            }
            break;

            case WindowMessageType::key_up:
            {
                WindowMessageKeyCode key_code = message.key_down_data.key_code;

                if (key_code == WindowMessageKeyCode::key_d)
                {
                    input.keydown_d = false;
                    input.keyup_d = true;
                }
                else if (key_code == WindowMessageKeyCode::key_a)
                {
                    input.keydown_a = false;
                    input.keyup_a = true;
                }
                else if (key_code == WindowMessageKeyCode::key_q)
                {
                    input.keydown_q = false;
                    input.keyup_q = true;
                }
                else if (key_code == WindowMessageKeyCode::key_e)
                {
                    input.keydown_e = false;
                    input.keyup_e = true;
                }
                else if (key_code == WindowMessageKeyCode::key_w)
                {
                    input.keydown_w = false;
                    input.keyup_w = true;
                }
                else if (key_code == WindowMessageKeyCode::key_s)
                {
                    input.keydown_s = false;
                    input.keyup_s = true;
                }
                else if (key_code == WindowMessageKeyCode::key_i)
                {
                    input.keydown_i = false;
                    input.keyup_i = true;
                }
                else if (key_code == WindowMessageKeyCode::key_k)
                {
                    input.keydown_k = false;
                    input.keyup_k = true;
                }
                else if (key_code == WindowMessageKeyCode::key_l)
                {
                    input.keydown_l = false;
                    input.keyup_l = true;
                }
                else if (key_code == WindowMessageKeyCode::key_j)
                {
                    input.keydown_j = false;
                    input.keyup_j = true;
                }
                else if (key_code == WindowMessageKeyCode::key_o)
                {
                    input.keydown_o = false;
                    input.keyup_o = true;
                }
                else if (key_code == WindowMessageKeyCode::key_u)
                {
                    input.keydown_u = false;
                    input.keyup_u = true;
                }
            }
            break;

            case WindowMessageType::mouse_down:
            {
                if (message.mouse_down_data.button_type == WindowMessageMouseButtonType::left)
                {
                    input.click_left = true;
                    input.mouse_x = message.mouse_down_data.x;
                    input.mouse_y = message.mouse_down_data.y;
                }
                else if (message.mouse_down_data.button_type == WindowMessageMouseButtonType::right)
                {
                    input.click_right = true;
                }
            }
            break;

            case WindowMessageType::mouse_move:
            {
                input.mouse_x = message.mouse_move_data.x;
                input.mouse_y = message.mouse_move_data.y;
            }
            break;
            }
        }

        if (input.mouse_x != -1)
        {
            state.mouse_x = input.mouse_x;
        }
        if (input.mouse_y != -1)
        {
            state.mouse_y = input.mouse_y;
        }

        Mat4 inverse_projection;
        ASSERT(inverse(scene_uniform_data->projection, &inverse_projection));
        Mat4 inverse_view;
        ASSERT(inverse(scene_uniform_data->view, &inverse_view));

        Vec4 mouse_pos_clip = {(Real)state.mouse_x * 2 / window_width - 1, (Real)state.mouse_y * 2 / window_height - 1, 0, 1};
        Vec3 mouse_pos_world = vec3(perspective_divide(inverse_view * inverse_projection * mouse_pos_clip));
        Ray ray_world;
        ray_world.pos = camera.pos;
        ray_world.dir = normalize(mouse_pos_world - camera.pos);

        // NOTE: Calculate hovered piece
        GamePiece *hover_piece = null;
        Real min_dist = 10000000;
        for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
        {
            EntityUniformData *uniform_data = get_piece_uniform_data(&device, &scene_uniform_buffer, piece_i);
            Mat4 inverse_world;
            ASSERT(inverse(uniform_data->world, &inverse_world));

            Vec3 camera_pos_local = vec3(inverse_world * vec4(camera.pos));
            Vec3 mouse_pos_local = vec3(inverse_world * vec4(mouse_pos_world));
            Ray ray_local;
            ray_local.pos = camera_pos_local;
            ray_local.dir = normalize(mouse_pos_local - camera_pos_local);

            Piece *piece = &pieces[piece_i];
            Real box_dist = check_box_collision(&ray_local, &piece->mesh->collision_box);
            if (box_dist >= 0 && box_dist < min_dist)
            {
                Real convex_hull_dist = check_convex_hull_collision(&ray_local, piece->mesh->collision_hull_vertex_count, piece->mesh->collision_hull_vertex_data);
                if (convex_hull_dist >= 0)
                {
                    min_dist = convex_hull_dist;
                    hover_piece = &game_state.pieces[piece_i];
                }
            }
        }

        // NOTE: Calculate hovered square
        Int hover_row = -1;
        Int hover_column = -1;
        if (hover_piece)
        {
            hover_row = hover_piece->row;
            hover_column = hover_piece->column;
        }
        else
        {
            for (Int square_i = 0; square_i < 64; square_i++)
            {
                Real dist = check_box_collision(&ray_world, &board.collision_box[square_i]);
                if (dist > 0)
                {
                    hover_row = get_row(square_i);
                    hover_column = get_column(square_i);
                    break;
                }
            }
        }

        // NOTE: Select piece or change piece selection
        if (input.click_left)
        {
            if (hover_piece &&
                hover_piece != game_state.selected_piece &&
                is_friend(&game_state, hover_piece) &&
                pieces[hover_piece->index].animation_type == AnimationType::none)
            {
                if (game_state.selected_piece)
                {
                    stop_flash_animation(&pieces[game_state.selected_piece->index]);
                }

                game_state.selected_piece = hover_piece;
                start_flash_animation(&pieces[game_state.selected_piece->index]);
            }
        }
        // NOTE: Clear piece selection
        else if (input.click_right)
        {
            if (game_state.selected_piece)
            {
                stop_flash_animation(&pieces[game_state.selected_piece->index]);
                game_state.selected_piece = null;
            }
        }

        // NOTE: Check game move
        GameMoveCheck game_move_check;
        if (game_state.selected_piece && hover_row != -1 && hover_column != -1)
        {
            game_move_check = check_game_move(&game_state, game_state.selected_piece, hover_row, hover_column);
        }
        else
        {
            game_move_check.is_illegal = true;
        }

        // NOTE: Update game move
        if (input.click_left)
        {
            if (game_state.selected_piece && !game_move_check.is_illegal)
            {
                record_game_move(&game_state, game_move_check.move_type, game_state.selected_piece->row, game_state.selected_piece->column, hover_row, hover_column);

                Piece *piece = &pieces[game_state.selected_piece->index];
                stop_flash_animation(piece);
                if (game_state.selected_piece->type == GamePieceType::knight ||
                    game_move_check.move_type == GameMoveType::castling_king ||
                    game_move_check.move_type == GameMoveType::castling_queen)
                {
                    start_jump_animation(piece, game_state.selected_piece->row, game_state.selected_piece->column);
                }
                else
                {
                    start_move_animation(piece, game_state.selected_piece->row, game_state.selected_piece->column);
                }

                if (game_move_check.move_type == GameMoveType::castling_king ||
                    game_move_check.move_type == GameMoveType::castling_queen)
                {
                    Piece *castling_piece = &pieces[game_move_check.castling_piece->index];
                    start_move_animation(castling_piece, game_move_check.castling_piece->row, game_move_check.castling_piece->column);
                }

                if (game_move_check.captured_piece)
                {
                    Piece *captured_piece = &pieces[game_move_check.captured_piece->index];
                    start_capture_animation(captured_piece, &random_generator);
                }

                game_state.selected_piece = null;
            }
        }

        // NOTE: Calculate ghost piece
        Int ghost_piece_index = -1;
        if (game_state.selected_piece &&
            hover_row != -1 && hover_column != -1)
        {
            if (!is_occupied(&game_state, hover_row, hover_column))
            {
                ghost_piece_index = game_state.selected_piece->index;
                update_ghost_piece(&ghost_piece, &pieces[game_state.selected_piece->index], hover_row, hover_column);
                ghost_piece.shadowed_piece_index = -1;
            }
            else if (is_foe_occupied(&game_state, hover_row, hover_column))
            {
                ghost_piece_index = game_state.selected_piece->index;
                update_ghost_piece(&ghost_piece, &pieces[game_state.selected_piece->index], hover_row, hover_column);
                ghost_piece.shadowed_piece_index = game_state.board[hover_row][hover_column]->index;
            }
        }

        // NOTE: Clear ghost piece illegal move feedback
        if (hover_row != last_hover_row || hover_column != last_hover_column ||
            ghost_piece_index == -1)
        {
            stop_illegal_flash_animation(&ghost_piece);
        }

        // NOTE: Start ghost piece illegal move feedback
        if (input.click_left)
        {
            if (ghost_piece_index != -1 && game_move_check.is_illegal)
            {
                start_illegal_flash_animation(&ghost_piece);
            }
        }

        last_hover_row = hover_row;
        last_hover_column = hover_column;

        calculate_scene_uniform_data(&camera, window_width, window_height, scene_uniform_data);
        for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
        {
            Piece *piece = &pieces[piece_i];
            update_animation(piece, frame_time);
            calculate_entity_uniform_data(piece, get_piece_uniform_data(&device, &scene_uniform_buffer, piece_i));
        }
        if (ghost_piece_index != -1)
        {
            update_animation(&ghost_piece, frame_time);
            calculate_entity_uniform_data(&ghost_piece, get_ghost_piece_uniform_data(&device, &scene_uniform_buffer));
        }

        // NOTE: Debug support
        DebugUIDrawState debug_ui_draw_state = {};
        if (input.keydown_g)
        {
            state.debug.show_debug_ui = !state.debug.show_debug_ui;
        }
        if (state.debug.show_debug_ui)
        {
            if (input.keydown_z)
            {
                camera = get_scene_camera();
            }
            else
            {
                if (input.keydown_d)
                {
                    state.debug.moving_x_pos = true;
                }
                else if (input.keyup_d)
                {
                    state.debug.moving_x_pos = false;
                }
                if (input.keydown_a)
                {
                    state.debug.moving_x_neg = true;
                }
                else if (input.keyup_a)
                {
                    state.debug.moving_x_neg = false;
                }
                if (input.keydown_q)
                {
                    state.debug.moving_y_pos = true;
                }
                else if (input.keyup_q)
                {
                    state.debug.moving_y_pos = false;
                }
                if (input.keydown_e)
                {
                    state.debug.moving_y_neg = true;
                }
                else if (input.keyup_e)
                {
                    state.debug.moving_y_neg = false;
                }
                if (input.keydown_w)
                {
                    state.debug.moving_z_pos = true;
                }
                else if (input.keyup_w)
                {
                    state.debug.moving_z_pos = false;
                }
                if (input.keydown_s)
                {
                    state.debug.moving_z_neg = true;
                }
                else if (input.keyup_s)
                {
                    state.debug.moving_z_neg = false;
                }

                if (input.keydown_i)
                {
                    state.debug.rotating_x_pos = true;
                }
                else if (input.keyup_i)
                {
                    state.debug.rotating_x_pos = false;
                }
                if (input.keydown_k)
                {
                    state.debug.rotating_x_neg = true;
                }
                else if (input.keyup_k)
                {
                    state.debug.rotating_x_neg = false;
                }
                if (input.keydown_l)
                {
                    state.debug.rotating_y_pos = true;
                }
                else if (input.keyup_l)
                {
                    state.debug.rotating_y_pos = false;
                }
                if (input.keydown_j)
                {
                    state.debug.rotating_y_neg = true;
                }
                else if (input.keyup_j)
                {
                    state.debug.rotating_y_neg = false;
                }
                if (input.keydown_o)
                {
                    state.debug.rotating_z_pos = true;
                }
                else if (input.keyup_o)
                {
                    state.debug.rotating_z_pos = false;
                }
                if (input.keydown_u)
                {
                    state.debug.rotating_z_neg = true;
                }
                else if (input.keyup_u)
                {
                    state.debug.rotating_z_neg = false;
                }

                Vec3 camera_x = rotate(camera.rot, get_basis_x());
                Vec3 camera_y = rotate(camera.rot, get_basis_y());
                Vec3 camera_z = rotate(camera.rot, get_basis_z());
                Real speed = 8;
                if (state.debug.moving_x_pos)
                {
                    camera.pos = camera.pos + speed * camera_x;
                }
                if (state.debug.moving_x_neg)
                {
                    camera.pos = camera.pos - speed * camera_x;
                }
                if (state.debug.moving_y_pos)
                {
                    camera.pos = camera.pos + speed * camera_y;
                }
                if (state.debug.moving_y_neg)
                {
                    camera.pos = camera.pos - speed * camera_y;
                }
                if (state.debug.moving_z_pos)
                {
                    camera.pos = camera.pos + speed * camera_z;
                }
                if (state.debug.moving_z_neg)
                {
                    camera.pos = camera.pos - speed * camera_z;
                }

                Real rotating_speed = degree_to_radian(0.5);
                Quaternion local_rot = get_identity_quaternion();
                if (state.debug.rotating_x_pos)
                {
                    local_rot = get_rotation_quaternion(get_basis_x(), rotating_speed) * local_rot;
                }
                if (state.debug.rotating_x_neg)
                {
                    local_rot = get_rotation_quaternion(get_basis_x(), -rotating_speed) * local_rot;
                }
                if (state.debug.rotating_y_pos)
                {
                    local_rot = get_rotation_quaternion(get_basis_y(), rotating_speed) * local_rot;
                }
                if (state.debug.rotating_y_neg)
                {
                    local_rot = get_rotation_quaternion(get_basis_y(), -rotating_speed) * local_rot;
                }
                if (state.debug.rotating_z_pos)
                {
                    local_rot = get_rotation_quaternion(get_basis_z(), rotating_speed) * local_rot;
                }
                if (state.debug.rotating_z_neg)
                {
                    local_rot = get_rotation_quaternion(get_basis_z(), -rotating_speed) * local_rot;
                }
                camera.rot = camera.rot * local_rot;
            }

            Vec2 debug_ui_start_pos = {-0.95, -0.95};
            debug_ui_draw_state = create_debug_ui_draw_state(&debug_font, window_width, window_height, &debug_ui_vertex_buffer, debug_ui_start_pos);

            debug_ui_draw_str(&debug_ui_draw_state, str("camera"));
            debug_ui_draw_newline(&debug_ui_draw_state);
            debug_ui_draw_indent(&debug_ui_draw_state, 1);

            debug_ui_draw_str(&debug_ui_draw_state, str("position: "));
            debug_ui_draw_vec3(&debug_ui_draw_state, camera.pos);
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("rotation: "));
            debug_ui_draw_vec4(&debug_ui_draw_state, vec4(camera.rot));
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_indent(&debug_ui_draw_state, -1);

            debug_ui_draw_str(&debug_ui_draw_state, str("selected piece: "));
            if (game_state.selected_piece)
            {
                debug_ui_draw_str(&debug_ui_draw_state, game_state.selected_piece->name);
            }
            else
            {
                debug_ui_draw_str(&debug_ui_draw_state, str("<no>"));
            }
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("selected square: "));
            if (hover_row != -1 && hover_column != -1)
            {
                debug_ui_draw_int(&debug_ui_draw_state, hover_row + 1);
                debug_ui_draw_str(&debug_ui_draw_state, str(", "));
                debug_ui_draw_int(&debug_ui_draw_state, hover_column + 1);
            }
            else
            {
                debug_ui_draw_str(&debug_ui_draw_state, str("<no>"));
            }
            debug_ui_draw_newline(&debug_ui_draw_state);
        }

        ASSERT(render_vulkan_frame(&device,
                                   &scene_pipeline, &scene_frame, &scene_uniform_buffer, &board, pieces, ghost_piece_index != -1 ? &ghost_piece : null,
                                   &shadow_pipeline, &shadow_frame,
                                   &debug_ui_pipeline, &debug_ui_frame, &debug_ui_vertex_buffer, debug_ui_draw_state.character_count,
                                   &debug_collision_pipeline, &debug_collision_frame, &debug_collision_vertex_buffer));

        UInt64 current_timestamp = get_current_timestamp();
        Real64 elapsed_time = get_elapsed_time(current_timestamp - last_timestamp);
        while (elapsed_time < frame_time)
        {
            sleep(frame_time - elapsed_time);
            current_timestamp = get_current_timestamp();
            elapsed_time = get_elapsed_time(current_timestamp - last_timestamp);
        }
        last_timestamp = current_timestamp;
    }

    return 0;
}

#include "../lib/util.cpp"
#include "../lib/vulkan.cpp"
#include "../lib/os.cpp"
