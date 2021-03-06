#include "comdef.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
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
#include "debug_move.cpp"
#include "blur.cpp"
#include "menu.cpp"
#include "search.cpp"

Str get_game_piece_name(GamePiece piece)
{
    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    switch (piece_type)
    {
    case GamePieceType::pawn:
    {
        return side == GameSide::white ? str("white pawn") : str("black pawn");
    }
    break;

    case GamePieceType::knight:
    {
        return side == GameSide::white ? str("white knight") : str("black knight");
    }
    break;

    case GamePieceType::bishop:
    {
        return side == GameSide::white ? str("white bishop") : str("black bishop");
    }
    break;

    case GamePieceType::rook:
    {
        return side == GameSide::white ? str("white rook") : str("black rook");
    }
    break;

    case GamePieceType::queen:
    {
        return side == GameSide::white ? str("white queen") : str("black queen");
    }
    break;

    case GamePieceType::king:
    {
        return side == GameSide::white ? str("white king") : str("black king");
    }
    break;

    default:
    {
        ASSERT(false);
        return str("");
    }
    break;
    }
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

struct BlurPushConstants
{
    UInt32 mode;
    Real overlay;
};

Bool render_vulkan_frame(VulkanDevice *device,
                         VulkanPipeline *scene_pipeline, SceneFrame *scene_frame, VulkanBuffer *scene_uniform_buffer,
                         Board *board, PieceManager *piece_manager, GhostPiece *ghost_piece,
                         VulkanPipeline *shadow_pipeline, ShadowFrame *shadow_frame,
                         Bool show_debug,
                         VulkanPipeline *debug_ui_pipeline, DebugUIFrame *debug_ui_frame, VulkanBuffer *debug_ui_vertex_buffer, Int debug_ui_character_count,
                         VulkanPipeline *debug_collision_pipeline, DebugCollisionFrame *debug_collision_frame, VulkanBuffer *debug_collision_vertex_buffer,
                         VulkanPipeline *debug_move_pipeline, DebugMoveFrame *debug_move_frame, VulkanBuffer *debug_move_vertex_buffer, Int debug_move_count,
                         VulkanPipeline *blur_pipeline, BlurFrame *blur_frame, Int blur_times,
                         VulkanPipeline *menu_pipeline, MenuFrame *menu_frame, VulkanBuffer *menu_vertex_buffer, Int menu_vertex_count)
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

    Int present_image_index;
    result_code = vkAcquireNextImageKHR(device->handle, device->swapchain.handle, UINT64_MAX, scene_frame->image_aquired_semaphore, VK_NULL_HANDLE, (UInt32 *)&present_image_index);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }
    VkImage present_image = device->swapchain.images[present_image_index];

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

    // NOTE: Shadow
    VkClearValue depth_clear_color = {1.0, 0};
    VkRenderPassBeginInfo shadow_render_pass_begin_info = {};
    shadow_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    shadow_render_pass_begin_info.renderPass = shadow_pipeline->render_pass;
    shadow_render_pass_begin_info.framebuffer = shadow_frame->frame_buffer;
    shadow_render_pass_begin_info.renderArea.offset = {0, 0};
    shadow_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
    shadow_render_pass_begin_info.clearValueCount = 1;
    shadow_render_pass_begin_info.pClearValues = &depth_clear_color;

    vkCmdBeginRenderPass(scene_frame->command_buffer, &shadow_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline->handle);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &scene_frame->vertex_buffer.handle, &offset);
    vkCmdBindIndexBuffer(scene_frame->command_buffer, scene_frame->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline->layout, 0, 1, &scene_frame->scene_descriptor_set, 0, null);

    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline->layout, 1, 1, &scene_frame->board_descriptor_set, 0, null);
    vkCmdDrawIndexed(scene_frame->command_buffer, board->mesh->index_count, 1, board->mesh->index_offset, board->mesh->vertex_offset, 0);

    for (Int piece_i = 0; piece_i < ENTITY_PIECE_COUNT; piece_i++)
    {
        Piece *piece = &piece_manager->pieces[piece_i];
        if (piece->square != NO_SQUARE)
        {
            Mesh *mesh = piece->mesh;
            vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline->layout, 1, 1, &scene_frame->piece_descriptor_sets[piece_i], 0, null);
            vkCmdDrawIndexed(scene_frame->command_buffer, mesh->index_count, 1, mesh->index_offset, mesh->vertex_offset, 0);
        }
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
    shadow_depth_image_memory_barrier.subresourceRange.levelCount = 1;
    shadow_depth_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    shadow_depth_image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, null, 0, null, 1, &shadow_depth_image_memory_barrier);

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
    depth_image_memory_barrier.subresourceRange.levelCount = 1;
    depth_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    depth_image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, null, 0, null, 1, &depth_image_memory_barrier);

    VkImageMemoryBarrier color_image_memory_barrier = {};
    color_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    color_image_memory_barrier.srcAccessMask = 0;
    color_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    color_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    color_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    color_image_memory_barrier.image = scene_frame->color_image;
    color_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    color_image_memory_barrier.subresourceRange.levelCount = 1;
    color_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    color_image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 1, &color_image_memory_barrier);

    VkImageMemoryBarrier multisample_color_image_memory_barrier = {};
    multisample_color_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    multisample_color_image_memory_barrier.srcAccessMask = 0;
    multisample_color_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    multisample_color_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    multisample_color_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    multisample_color_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    multisample_color_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    multisample_color_image_memory_barrier.image = scene_frame->multisample_color_image;
    multisample_color_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    multisample_color_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    multisample_color_image_memory_barrier.subresourceRange.levelCount = 1;
    multisample_color_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    multisample_color_image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 1, &multisample_color_image_memory_barrier);

    VkClearColorValue clear_color = {0.7, 0.7, 0.7, 1};
    vkCmdClearColorImage(scene_frame->command_buffer, scene_frame->multisample_color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &multisample_color_image_memory_barrier.subresourceRange);

    multisample_color_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    multisample_color_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    multisample_color_image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    multisample_color_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    multisample_color_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    multisample_color_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    multisample_color_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    multisample_color_image_memory_barrier.image = scene_frame->multisample_color_image;
    multisample_color_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    multisample_color_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    multisample_color_image_memory_barrier.subresourceRange.levelCount = 1;
    multisample_color_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    multisample_color_image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, null, 0, null, 1, &multisample_color_image_memory_barrier);

    // NOTE: Board
    VkClearValue scene_clear_colors[2] = {{}, depth_clear_color};
    VkRenderPassBeginInfo scene_render_pass_begin_info = {};
    scene_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    scene_render_pass_begin_info.renderPass = scene_pipeline->render_pass;
    scene_render_pass_begin_info.framebuffer = scene_frame->frame_buffer;
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
    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 2, 1, &board->light_map->descriptor_set, 0, null);
    vkCmdDrawIndexed(scene_frame->command_buffer, board->mesh->index_count, 1, board->mesh->index_offset, board->mesh->vertex_offset, 0);

    vkCmdEndRenderPass(scene_frame->command_buffer);

    // NOTE: Debug Move
    if (show_debug && debug_move_count > 0)
    {
        VkBufferCopy debug_move_vertex_buffer_copy = {};
        debug_move_vertex_buffer_copy.srcOffset = 0;
        debug_move_vertex_buffer_copy.dstOffset = 0;
        debug_move_vertex_buffer_copy.size = sizeof(DebugMoveVertex) * debug_move_count * 6;
        vkCmdCopyBuffer(scene_frame->command_buffer, debug_move_vertex_buffer->handle, debug_move_frame->vertex_buffer.handle, 1, &debug_move_vertex_buffer_copy);

        VkBufferMemoryBarrier debug_move_vertex_buffer_memory_barrier = {};
        debug_move_vertex_buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        debug_move_vertex_buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        debug_move_vertex_buffer_memory_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        debug_move_vertex_buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        debug_move_vertex_buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        debug_move_vertex_buffer_memory_barrier.buffer = debug_move_frame->vertex_buffer.handle;
        debug_move_vertex_buffer_memory_barrier.offset = 0;
        debug_move_vertex_buffer_memory_barrier.size = sizeof(DebugMoveVertex) * debug_move_count * 6;
        vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, null, 1, &debug_move_vertex_buffer_memory_barrier, 0, null);

        VkRenderPassBeginInfo debug_move_render_pass_begin_info = {};
        debug_move_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        debug_move_render_pass_begin_info.renderPass = debug_move_pipeline->render_pass;
        debug_move_render_pass_begin_info.framebuffer = debug_move_frame->frame_buffer;
        debug_move_render_pass_begin_info.renderArea.offset = {0, 0};
        debug_move_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
        debug_move_render_pass_begin_info.clearValueCount = 0;
        debug_move_render_pass_begin_info.pClearValues = null;

        vkCmdBeginRenderPass(scene_frame->command_buffer, &debug_move_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_move_pipeline->handle);

        offset = 0;
        vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &debug_move_frame->vertex_buffer.handle, &offset);

        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_move_pipeline->layout, 0, 1, &scene_frame->scene_descriptor_set, 0, null);

        vkCmdDraw(scene_frame->command_buffer, debug_move_count * 6, 1, 0, 0);

        vkCmdEndRenderPass(scene_frame->command_buffer);
    }

    // NOTE: Pieces
    vkCmdBeginRenderPass(scene_frame->command_buffer, &scene_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->handle);

    offset = 0;
    vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &scene_frame->vertex_buffer.handle, &offset);
    vkCmdBindIndexBuffer(scene_frame->command_buffer, scene_frame->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 0, 1, &scene_frame->scene_descriptor_set, 0, null);
    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 3, 1, &scene_frame->shadow_descriptor_set, 0, null);

    for (Int piece_i = 0; piece_i < ENTITY_PIECE_COUNT; piece_i++)
    {
        Piece *piece = &piece_manager->pieces[piece_i];
        if (piece->square != NO_SQUARE && (ghost_piece == null || piece != ghost_piece->shadowed_piece))
        {
            Mesh *mesh = piece->mesh;
            Image *light_map = piece->light_map;
            vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 1, 1, &scene_frame->piece_descriptor_sets[piece_i], 0, null);
            vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 2, 1, &light_map->descriptor_set, 0, null);
            vkCmdDrawIndexed(scene_frame->command_buffer, mesh->index_count, 1, mesh->index_offset, mesh->vertex_offset, 0);
        }
    }

    if (ghost_piece)
    {
        Mesh *mesh = ghost_piece->mesh;
        Image *light_map = ghost_piece->light_map;
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 1, 1, &scene_frame->ghost_piece_descriptor_set, 0, null);
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 2, 1, &light_map->descriptor_set, 0, null);
        vkCmdDrawIndexed(scene_frame->command_buffer, mesh->index_count, 1, mesh->index_offset, mesh->vertex_offset, 0);
    }

    vkCmdEndRenderPass(scene_frame->command_buffer);

    // NOTE: Debug collision
    if (show_debug)
    {
        VkRenderPassBeginInfo debug_collision_render_pass_begin_info = {};
        debug_collision_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        debug_collision_render_pass_begin_info.renderPass = debug_collision_pipeline->render_pass;
        debug_collision_render_pass_begin_info.framebuffer = debug_collision_frame->frame_buffer;
        debug_collision_render_pass_begin_info.renderArea.offset = {0, 0};
        debug_collision_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
        debug_collision_render_pass_begin_info.clearValueCount = 0;
        debug_collision_render_pass_begin_info.pClearValues = null;

        vkCmdBeginRenderPass(scene_frame->command_buffer, &debug_collision_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_collision_pipeline->handle);

        offset = 0;
        vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &debug_collision_frame->vertex_buffer.handle, &offset);

        Int vertex_offset = 0;
        for (Int piece_i = 0; piece_i < ENTITY_PIECE_COUNT; piece_i++)
        {
            Piece *piece = &piece_manager->pieces[piece_i];
            Mesh *mesh = piece->mesh;
            Int vertex_count = COLLISION_BOX_VERTEX_COUNT + mesh->collision_hull_vertex_count * 2;
            if (piece->square != NO_SQUARE)
            {
                vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_collision_pipeline->layout, 1, 1, &scene_frame->piece_descriptor_sets[piece_i], 0, null);
                vkCmdDraw(scene_frame->command_buffer, vertex_count, 1, vertex_offset, 0);
            }
            vertex_offset += vertex_count;
        }

        vkCmdEndRenderPass(scene_frame->command_buffer);
    }

    // NOTE: Blur
    if (blur_times)
    {
        ASSERT(blur_times > 0 && blur_times <= MAX_BLUR_TIMES);
        vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, blur_pipeline->handle);

        offset = 0;
        vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &blur_frame->vertex_buffer.handle, &offset);

        VkRenderPassBeginInfo blur_render_pass_begin_info = {};
        blur_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        blur_render_pass_begin_info.renderPass = blur_pipeline->render_pass;
        blur_render_pass_begin_info.renderArea.offset = {0, 0};
        blur_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
        blur_render_pass_begin_info.clearValueCount = 0;
        blur_render_pass_begin_info.pClearValues = null;

        VkImage blur_from_image = blur_times > 1 ? blur_frame->blur_images[blur_times - 2] : scene_frame->color_image;
        VkDescriptorSet blur_from_descriptor_set = blur_times > 1 ? blur_frame->blur_descriptor_sets[blur_times - 2] : blur_frame->color_descriptor_set;

        VkImage blur_image = blur_frame->blur_images[blur_times - 1];
        VkFramebuffer blur_framebuffer = blur_frame->blur_framebuffer[blur_times - 1];
        VkDescriptorSet blur_descriptor_set = blur_frame->blur_descriptor_sets[blur_times - 1];

        VkImage blur_middle_image = blur_frame->blur_images[MAX_BLUR_TIMES];
        VkFramebuffer blur_middle_framebuffer = blur_frame->blur_framebuffer[MAX_BLUR_TIMES];
        VkDescriptorSet blur_middle_descriptor_set = blur_frame->blur_descriptor_sets[MAX_BLUR_TIMES];

        BlurPushConstants blur_push_constants;

        VkImageMemoryBarrier attachment_to_texture_image_memory_barrier = {};
        attachment_to_texture_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        attachment_to_texture_image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        attachment_to_texture_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        attachment_to_texture_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_to_texture_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachment_to_texture_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        attachment_to_texture_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        attachment_to_texture_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        attachment_to_texture_image_memory_barrier.subresourceRange.baseMipLevel = 0;
        attachment_to_texture_image_memory_barrier.subresourceRange.levelCount = 1;
        attachment_to_texture_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        attachment_to_texture_image_memory_barrier.subresourceRange.layerCount = 1;

        VkImageMemoryBarrier to_attachment_image_memory_barrier = {};
        to_attachment_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_attachment_image_memory_barrier.srcAccessMask = 0;
        to_attachment_image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        to_attachment_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        to_attachment_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        to_attachment_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_attachment_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_attachment_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        to_attachment_image_memory_barrier.subresourceRange.baseMipLevel = 0;
        to_attachment_image_memory_barrier.subresourceRange.levelCount = 1;
        to_attachment_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        to_attachment_image_memory_barrier.subresourceRange.layerCount = 1;

        if (blur_times == 1)
        {
            attachment_to_texture_image_memory_barrier.image = blur_from_image;
            vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, null, 0, null, 1, &attachment_to_texture_image_memory_barrier);
        }

        // NOTE: Horizontal blur
        to_attachment_image_memory_barrier.image = blur_middle_image;
        vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, null, 0, null, 1, &to_attachment_image_memory_barrier);

        blur_render_pass_begin_info.framebuffer = blur_middle_framebuffer;
        vkCmdBeginRenderPass(scene_frame->command_buffer, &blur_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, blur_pipeline->layout, 0, 1, &blur_from_descriptor_set, 0, null);
        blur_push_constants.mode = BLUR_MODE_HORIZONTAL;
        vkCmdPushConstants(scene_frame->command_buffer, blur_pipeline->layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlurPushConstants), &blur_push_constants);
        vkCmdDraw(scene_frame->command_buffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(scene_frame->command_buffer);

        // NOTE: Vertical blur
        attachment_to_texture_image_memory_barrier.image = blur_middle_image;
        vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, null, 0, null, 1, &attachment_to_texture_image_memory_barrier);
        to_attachment_image_memory_barrier.image = blur_image;
        vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, null, 0, null, 1, &to_attachment_image_memory_barrier);

        blur_render_pass_begin_info.framebuffer = blur_framebuffer;
        vkCmdBeginRenderPass(scene_frame->command_buffer, &blur_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, blur_pipeline->layout, 0, 1, &blur_middle_descriptor_set, 0, null);
        blur_push_constants.mode = BLUR_MODE_VERTICAL;
        vkCmdPushConstants(scene_frame->command_buffer, blur_pipeline->layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlurPushConstants), &blur_push_constants);
        vkCmdDraw(scene_frame->command_buffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(scene_frame->command_buffer);

        // NOTE: Overlay
        attachment_to_texture_image_memory_barrier.image = blur_image;
        vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, null, 0, null, 1, &attachment_to_texture_image_memory_barrier);
        to_attachment_image_memory_barrier.image = scene_frame->color_image;
        vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, null, 0, null, 1, &to_attachment_image_memory_barrier);

        blur_render_pass_begin_info.framebuffer = blur_frame->color_framebuffer;
        vkCmdBeginRenderPass(scene_frame->command_buffer, &blur_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, blur_pipeline->layout, 0, 1, &blur_descriptor_set, 0, null);
        blur_push_constants.mode = BLUR_MODE_OVERLAY;
        blur_push_constants.overlay = calculate_blur_overlay(blur_times);
        vkCmdPushConstants(scene_frame->command_buffer, blur_pipeline->layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlurPushConstants), &blur_push_constants);
        vkCmdDraw(scene_frame->command_buffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(scene_frame->command_buffer);
    }

    // NOTE: Menu
    if (blur_times && menu_vertex_count)
    {
        VkBufferCopy menu_vertex_buffer_copy = {};
        menu_vertex_buffer_copy.srcOffset = 0;
        menu_vertex_buffer_copy.dstOffset = 0;
        menu_vertex_buffer_copy.size = menu_vertex_buffer->count;
        vkCmdCopyBuffer(scene_frame->command_buffer, menu_vertex_buffer->handle, menu_frame->vertex_buffer.handle, 1, &menu_vertex_buffer_copy);

        VkBufferMemoryBarrier menu_vertex_buffer_memory_barrier = {};
        menu_vertex_buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        menu_vertex_buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        menu_vertex_buffer_memory_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        menu_vertex_buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        menu_vertex_buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        menu_vertex_buffer_memory_barrier.buffer = menu_frame->vertex_buffer.handle;
        menu_vertex_buffer_memory_barrier.offset = 0;
        menu_vertex_buffer_memory_barrier.size = menu_vertex_buffer->count;
        vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, null, 1, &menu_vertex_buffer_memory_barrier, 0, null);

        VkRenderPassBeginInfo menu_render_pass_begin_info = {};
        menu_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        menu_render_pass_begin_info.renderPass = menu_pipeline->render_pass;
        menu_render_pass_begin_info.framebuffer = menu_frame->frame_buffer;
        menu_render_pass_begin_info.renderArea.offset = {0, 0};
        menu_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
        menu_render_pass_begin_info.clearValueCount = 0;
        menu_render_pass_begin_info.pClearValues = null;

        vkCmdBeginRenderPass(scene_frame->command_buffer, &menu_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, menu_pipeline->handle);

        offset = 0;
        vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &menu_frame->vertex_buffer.handle, &offset);

        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, menu_pipeline->layout, 0, 1, &menu_frame->font_texture_descriptor_set, 0, null);

        vkCmdDraw(scene_frame->command_buffer, menu_vertex_count, 1, 0, 0);

        vkCmdEndRenderPass(scene_frame->command_buffer);
    }

    // NOTE: Debug UI
    if (show_debug)
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
        debug_ui_render_pass_begin_info.framebuffer = debug_ui_frame->frame_buffer;
        debug_ui_render_pass_begin_info.renderArea.offset = {0, 0};
        debug_ui_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
        debug_ui_render_pass_begin_info.clearValueCount = 0;
        debug_ui_render_pass_begin_info.pClearValues = null;

        vkCmdBeginRenderPass(scene_frame->command_buffer, &debug_ui_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_ui_pipeline->handle);

        offset = 0;
        vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &debug_ui_frame->vertex_buffer.handle, &offset);

        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_ui_pipeline->layout, 0, 1, &debug_ui_frame->font_texture_descriptor_set, 0, null);

        vkCmdDraw(scene_frame->command_buffer, debug_ui_character_count * 6, 1, 0, 0);

        vkCmdEndRenderPass(scene_frame->command_buffer);
    }

    // NOTE: Copy
    color_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    color_image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    color_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    color_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    color_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    color_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    color_image_memory_barrier.image = scene_frame->color_image;
    color_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    color_image_memory_barrier.subresourceRange.levelCount = 1;
    color_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    color_image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 1, &color_image_memory_barrier);

    VkImageMemoryBarrier present_image_memory_barrier = {};
    present_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    present_image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    present_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    present_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    present_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    present_image_memory_barrier.srcQueueFamilyIndex = device->present_queue_index;
    present_image_memory_barrier.dstQueueFamilyIndex = device->graphics_queue_index;
    present_image_memory_barrier.image = present_image;
    present_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    present_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    present_image_memory_barrier.subresourceRange.levelCount = 1;
    present_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    present_image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 1, &present_image_memory_barrier);

    VkImageCopy present_image_copy;
    present_image_copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    present_image_copy.srcSubresource.mipLevel = 0;
    present_image_copy.srcSubresource.baseArrayLayer = 0;
    present_image_copy.srcSubresource.layerCount = 1;
    present_image_copy.srcOffset = {0, 0};
    present_image_copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    present_image_copy.dstSubresource.mipLevel = 0;
    present_image_copy.dstSubresource.baseArrayLayer = 0;
    present_image_copy.dstSubresource.layerCount = 1;
    present_image_copy.dstOffset = {0, 0};
    present_image_copy.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
    vkCmdCopyImage(scene_frame->command_buffer, scene_frame->color_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, present_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &present_image_copy);

    present_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    present_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    present_image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    present_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    present_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    present_image_memory_barrier.srcQueueFamilyIndex = device->graphics_queue_index;
    present_image_memory_barrier.dstQueueFamilyIndex = device->present_queue_index;
    present_image_memory_barrier.image = present_image;
    present_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    present_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    present_image_memory_barrier.subresourceRange.levelCount = 1;
    present_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    present_image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, null, 0, null, 1, &present_image_memory_barrier);

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
    present_info.pImageIndices = (UInt32 *)&present_image_index,
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
    Bool keydown_r;
    Bool keydown_esc;

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

namespace StatePhase
{
enum
{
    select,
    execute,
    animate,
    promote,
    ai_think,
    ai_animate,
    menu_blur,
    menu_unblur,
    menu,
};
}
typedef Int StatePhaseEnum;

Bool in_menu(StatePhaseEnum state_phase)
{
    Bool result = state_phase == StatePhase::menu_blur || state_phase == StatePhase::menu_unblur || state_phase == StatePhase::menu;
    return result;
}

struct State
{
    Int mouse_x;
    Int mouse_y;

    StatePhaseEnum phase;
    Piece *selected_piece;
    GameMove executing_move;
    Int promotion_index;
    Square last_hover_square;
    StatePhaseEnum phase_before_menu;
    Int blur_times;
    MenuState menu_state;
    GameEndEnum menu_game_end;

    Bool show_debug;
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

Void reset_state(State *state, GameSideEnum player_side)
{
    state->phase = StatePhase::select;
    state->menu_state.selected_player = player_side;
    state->menu_state.hovered_player = GameSide::count;
    state->menu_game_end = GameEnd::none;
}

Void check_end(State *state, GameState *game_state)
{
    GameEndEnum game_end = check_game_end(game_state);
    if (game_end != GameEnd::none)
    {
        state->menu_game_end = game_end;
        state->phase = StatePhase::menu_blur;
    }
}

struct SoundRenderer
{
    IAudioClient *audio_client;
    IAudioRenderClient *audio_render_client;
    Sound *sound;
    Int frame_pos;
};

SoundRenderer create_sound_renderer()
{
    // TODO: Sound error proof
    HRESULT result = CoInitialize(null);
    IMMDeviceEnumerator *audio_device_enumerator;
    result = CoCreateInstance(__uuidof(MMDeviceEnumerator), null, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&audio_device_enumerator);
    ASSERT(SUCCEEDED(result));
    IMMDevice *audio_device;
    result = audio_device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audio_device);
    ASSERT(SUCCEEDED(result));
    IAudioClient *audio_client;
    result = audio_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, null, (void **)&audio_client);
    ASSERT(SUCCEEDED(result));
    WAVEFORMATEX *audio_format;
    result = audio_client->GetMixFormat(&audio_format);
    ASSERT(SUCCEEDED(result));
    result = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 1 * 1000 * 1000 * 10, 0, audio_format, null);
    ASSERT(SUCCEEDED(result));
    UInt audio_buffer_size;
    result = audio_client->GetBufferSize(&audio_buffer_size);
    ASSERT(SUCCEEDED(result));
    IAudioRenderClient *audio_render_client;
    result = audio_client->GetService(__uuidof(IAudioRenderClient), (void **)&audio_render_client);

    SoundRenderer sound_renderer = {};
    sound_renderer.audio_client = audio_client;
    sound_renderer.audio_render_client = audio_render_client;
    return sound_renderer;
}

Void start_sound(SoundRenderer *sound_renderer, Sound *sound)
{
    if (sound_renderer->sound)
    {
        sound_renderer->audio_client->Stop();
        sound_renderer->audio_client->Reset();
    }
    sound_renderer->sound = sound;
    sound_renderer->frame_pos = 0;
}

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    AssetStore asset_store;
    ASSERT(load_asset(&asset_store));

    RandomGenerator random_generator;
    random_generator.seed = get_current_timestamp();
    initialize_zobrist_keys(&random_generator);

    GameSideEnum initial_player_side = GameSide::white;
    GameState game_state = get_initial_game_state(asset_store.bit_board_table, initial_player_side);

    Searcher searcher;
    ASSERT(initialize_searcher(&searcher, &game_state));
    ASSERT(run_thread((ThreadFunc)search, &searcher));

    Board board;
    fill_board_initial_state(&board, &asset_store.board_mesh, &asset_store.board_light_map);

    PieceManager piece_manager;
    piece_manager.asset_store = &asset_store;
    fill_piece_manager_initial_state(&piece_manager, &game_state);

    GhostPiece ghost_piece;
    fill_ghost_piece_initila_state(&ghost_piece);

    Int window_width = 800;
    Int window_height = 600;
    Window window = create_window(str("Chess"), window_width, window_height, 50, 50, false);
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

    VulkanPipeline debug_move_pipeline;
    ASSERT(create_debug_move_pipeline(&device, &debug_move_pipeline));

    VulkanPipeline blur_pipeline;
    ASSERT(create_blur_pipeline(&device, &blur_pipeline));

    VulkanPipeline menu_pipeline;
    ASSERT(create_menu_pipeline(&device, &menu_pipeline));

    ShadowFrame shadow_frame;
    ASSERT(create_shadow_frame(&device, &shadow_pipeline, &shadow_frame));

    SceneFrame scene_frame;
    VulkanBuffer scene_vertex_buffer;
    VulkanBuffer scene_index_buffer;
    VulkanBuffer scene_uniform_buffer;
    ASSERT(create_scene_frame(&device, &scene_pipeline, &board, &piece_manager, &asset_store, &shadow_frame, &scene_frame, &scene_vertex_buffer, &scene_index_buffer, &scene_uniform_buffer));

    DebugUIFrame debug_ui_frame;
    VulkanBuffer debug_ui_vertex_buffer;
    ASSERT(create_debug_ui_frame(&device, &debug_ui_pipeline, &asset_store.debug_font, &scene_frame, &debug_ui_frame, &debug_ui_vertex_buffer));

    DebugCollisionFrame debug_collision_frame;
    VulkanBuffer debug_collision_vertex_buffer;
    ASSERT(create_debug_collision_frame(&device, &debug_collision_pipeline, &piece_manager, &scene_frame, &debug_collision_frame, &debug_collision_vertex_buffer));

    DebugMoveFrame debug_move_frame;
    VulkanBuffer debug_move_vertex_buffer;
    ASSERT(create_debug_move_frame(&device, &debug_move_pipeline, &scene_frame, &debug_move_frame, &debug_move_vertex_buffer));

    BlurFrame blur_frame;
    ASSERT(create_blur_frame(&device, &blur_pipeline, &scene_frame, &blur_frame));

    MenuFrame menu_frame;
    VulkanBuffer menu_vertex_buffer;
    ASSERT(create_menu_frame(&device, &menu_pipeline, &scene_frame, &menu_frame, asset_store.menu_fonts, &menu_vertex_buffer));

    Camera camera = get_scene_camera(game_state.player_side);

    SceneUniformData *scene_uniform_data = get_scene_uniform_data(&device, &scene_uniform_buffer);
    calculate_scene_uniform_data(&camera, window_width, window_height, scene_uniform_data);

    calculate_entity_uniform_data(&board, get_board_uniform_data(&device, &scene_uniform_buffer));
    for (Int piece_i = 0; piece_i < ENTITY_PIECE_COUNT; piece_i++)
    {
        Piece *piece = &piece_manager.pieces[piece_i];
        calculate_entity_uniform_data(piece, get_piece_uniform_data(&device, &scene_uniform_buffer, piece_i));
    }

    write_entity_vertex_data(&board, &scene_vertex_buffer, &scene_index_buffer);
    for (Int piece_i = 0; piece_i < ENTITY_PIECE_COUNT; piece_i++)
    {
        write_entity_vertex_data(&piece_manager.pieces[piece_i], &scene_vertex_buffer, &scene_index_buffer);
    }
    ASSERT(upload_buffer(&device, &scene_vertex_buffer, &scene_frame.vertex_buffer));
    ASSERT(upload_buffer(&device, &scene_index_buffer, &scene_frame.index_buffer));

    DebugCollisionVertex *vertex = (DebugCollisionVertex *)debug_collision_vertex_buffer.data;
    for (Int piece_i = 0; piece_i < ENTITY_PIECE_COUNT; piece_i++)
    {
        vertex = write_collision_data(piece_manager.pieces[piece_i].mesh, vertex);
    }
    ASSERT(upload_buffer(&device, &debug_collision_vertex_buffer, &debug_collision_frame.vertex_buffer));

    State state = {};
    reset_state(&state, game_state.player_side);

    MenuLayout menu_layout = get_menu_layout(asset_store.menu_fonts, window_width, window_height);

    SoundRenderer sound_renderer = create_sound_renderer();

    show_window(window);

    Bool is_running = true;
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
                    input.keyup_d = false;
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
                else if (key_code == WindowMessageKeyCode::key_r)
                {
                    input.keydown_r = true;
                }
                else if (key_code == WindowMessageKeyCode::key_esc)
                {
                    input.keydown_esc = true;
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
        Piece *hover_piece = null;
        Real min_dist = 10000000;
        for (Int piece_i = 0; piece_i < ENTITY_PIECE_COUNT; piece_i++)
        {
            Piece *piece = &piece_manager.pieces[piece_i];
            if (piece->square != NO_SQUARE)
            {
                EntityUniformData *uniform_data = get_piece_uniform_data(&device, &scene_uniform_buffer, piece_i);
                Mat4 inverse_world;
                ASSERT(inverse(uniform_data->world, &inverse_world));

                Vec3 camera_pos_local = vec3(inverse_world * vec4(camera.pos));
                Vec3 mouse_pos_local = vec3(inverse_world * vec4(mouse_pos_world));
                Ray ray_local;
                ray_local.pos = camera_pos_local;
                ray_local.dir = normalize(mouse_pos_local - camera_pos_local);

                Real box_dist = check_box_collision(&ray_local, &piece->mesh->collision_box);
                if (box_dist >= 0 && box_dist < min_dist)
                {
                    Real convex_hull_dist = check_convex_hull_collision(&ray_local, piece->mesh->collision_hull_vertex_count, piece->mesh->collision_hull_vertex_data);
                    if (convex_hull_dist >= 0)
                    {
                        min_dist = convex_hull_dist;
                        hover_piece = piece;
                    }
                }
            }
        }

        // NOTE: Calculate hovered square
        Square hover_square = NO_SQUARE;
        if (hover_piece)
        {
            hover_square = hover_piece->square;
        }
        else
        {
            for (Square square = 0; square < 64; square++)
            {
                Real dist = check_box_collision(&ray_world, &board.collision_box[square]);
                if (dist > 0)
                {
                    hover_square = square;
                    break;
                }
            }
        }

        BitBoard all_moves = 0;
        Bool show_ghost_piece = false;

        if (input.keydown_esc)
        {
            switch (state.phase)
            {
            case StatePhase::menu:
            {
                state.phase = StatePhase::menu_unblur;
            }
            break;

            case StatePhase::menu_blur:
            {
                if (state.menu_game_end == GameEnd::none)
                {
                    state.phase = StatePhase::menu_unblur;
                }
            }
            break;

            case StatePhase::menu_unblur:
            {
                if (state.menu_game_end == GameEnd::none)
                {
                    state.phase = StatePhase::menu_blur;
                }
            }

            default:
            {
                state.phase_before_menu = state.phase;
                state.phase = StatePhase::menu_blur;
            }
            break;
            }
        }

        switch (state.phase)
        {
        case StatePhase::select:
        {
            ASSERT(!state.selected_piece);
            // NOTE: Undo
            if (input.keydown_z)
            {
                GameMove move1;
                GameMove move2;
                if (undo(&game_state, &move1))
                {
                    rollback_move(&piece_manager, move1);
                    if (undo(&game_state, &move2))
                    {
                        rollback_move(&piece_manager, move2);
                    }
                    else
                    {
                        redo(&game_state, &move1);
                        record_move(&piece_manager, move1);
                    }
                }
            }
            // NOTE: Redo
            else if (input.keydown_r)
            {
                GameMove move1;
                GameMove move2;
                if (redo(&game_state, &move1))
                {
                    record_move(&piece_manager, move1);
                    if (redo(&game_state, &move2))
                    {
                        record_move(&piece_manager, move2);
                    }
                    else
                    {
                        undo(&game_state, &move1);
                        rollback_move(&piece_manager, move1);
                    }
                }
            }
            // NOTE: Select piece
            else if (input.click_left)
            {
                if (hover_piece)
                {
                    GamePiece game_piece = game_state.board[hover_piece->square];
                    ASSERT(!is_empty(game_piece));
                    if (is_friend(&game_state, game_piece))
                    {
                        start_flash_animation(hover_piece);
                        state.selected_piece = hover_piece;
                        state.phase = StatePhase::execute;
                    }
                }
            }
        }
        break;

        case StatePhase::execute:
        {
            ASSERT(state.selected_piece);
            all_moves = check_game_move(&game_state, state.selected_piece->square);

            Bool selection_changed = false;
            Bool illegal_move = false;
            // NOTE: Change piece selection
            if (input.click_left && hover_piece)
            {
                GamePiece game_piece = game_state.board[hover_piece->square];
                ASSERT(!is_empty(game_piece));
                if (state.selected_piece->square != hover_piece->square && is_friend(&game_state, game_piece))
                {
                    stop_flash_animation(state.selected_piece);
                    start_flash_animation(hover_piece);
                    state.selected_piece = hover_piece;
                    selection_changed = true;
                }
            }
            // NOTE: Clear piece selection
            else if (input.click_right || input.keydown_z)
            {
                stop_flash_animation(state.selected_piece);
                state.selected_piece = null;
                state.phase = StatePhase::select;
            }

            // NOTE: Update game move
            if (state.phase == StatePhase::execute && !selection_changed)
            {
                if (input.click_left && hover_square != NO_SQUARE && HAS_FLAG(all_moves, bit_square(hover_square)))
                {
                    GameMove game_move = get_game_move(&game_state, state.selected_piece->square, hover_square);
                    if (is_move_legal(&game_state, game_move))
                    {
                        stop_flash_animation(state.selected_piece);
                        start_animation(&piece_manager, state.selected_piece, &game_state, game_move);
                        state.phase = StatePhase::animate;
                        state.executing_move = game_move;
                    }
                    else
                    {
                        illegal_move = true;
                    }
                }
            }

            // NOTE: Calculate ghost piece
            if (state.phase == StatePhase::execute && !selection_changed)
            {
                if (hover_square != NO_SQUARE)
                {
                    GamePiece game_piece = game_state.board[hover_square];
                    if (is_empty(game_piece))
                    {
                        show_ghost_piece = true;
                        update_ghost_piece(&ghost_piece, state.selected_piece, hover_square);
                        ghost_piece.shadowed_piece = null;
                    }
                    else if (is_foe(&game_state, game_piece))
                    {
                        show_ghost_piece = true;
                        update_ghost_piece(&ghost_piece, state.selected_piece, hover_square);
                        ghost_piece.shadowed_piece = piece_manager.piece_mapping[hover_square];
                        ASSERT(ghost_piece.shadowed_piece);
                    }

                    // NOTE: Start ghost piece illegal move feedback
                    if (input.click_left)
                    {
                        if (!HAS_FLAG(all_moves, bit_square(hover_square)) || illegal_move)
                        {
                            start_illegal_flash_animation(&ghost_piece);
                            start_sound(&sound_renderer, &asset_store.sound_error);
                        }
                    }
                    // NOTE: Clear ghost piece illegal move feedback
                    else if (!show_ghost_piece || hover_square != state.last_hover_square)
                    {
                        stop_illegal_flash_animation(&ghost_piece);
                    }
                }
            }
        }
        break;

        case StatePhase::animate:
        {
            ASSERT(state.selected_piece);
            if (state.selected_piece->animation_type == AnimationType::none)
            {
                GameMoveTypeEnum move_type = get_move_type(state.executing_move);
                // NOTE: Start promotion
                if (move_type == GameMoveType::promotion)
                {
                    state.promotion_index = 0;
                    GameSideEnum side = get_side(state.executing_move);
                    GamePiece promoted_piece = get_game_piece(side, promotion_list[state.promotion_index]);
                    set_piece_mesh(&piece_manager, state.selected_piece, promoted_piece);
                    start_flash_animation(state.selected_piece);
                    state.phase = StatePhase::promote;
                }
                // NOTE: Record game move
                else
                {
                    record_game_move_with_history(&game_state, state.executing_move);
                    record_move(&piece_manager, state.executing_move);
                    state.phase = StatePhase::select;
                    state.selected_piece = null;

                    check_end(&state, &game_state);
                }
            }
        }
        break;

        case StatePhase::promote:
        {
            // NOTE: Undo partial promotion
            if (input.keydown_z)
            {
                stop_flash_animation(state.selected_piece);
                GameMove promotion_move = add_promotion_index(state.executing_move, state.promotion_index);
                record_move(&piece_manager, promotion_move);
                rollback_move(&piece_manager, promotion_move);
                state.phase = StatePhase::select;
                state.selected_piece = null;
            }
            // NOTE: Confirm promotion
            else if (input.click_left)
            {
                stop_flash_animation(state.selected_piece);
                GameMove promotion_move = add_promotion_index(state.executing_move, state.promotion_index);
                record_game_move_with_history(&game_state, promotion_move);
                record_move(&piece_manager, promotion_move);
                state.phase = StatePhase::ai_think;
                state.selected_piece = null;

                check_end(&state, &game_state);
            }
            // NOTE: Change promotion
            else if (input.click_right)
            {
                state.promotion_index = (state.promotion_index + 1) % promotion_list.count;
                GamePiece game_piece = game_state.board[state.selected_piece->square];
                GamePiece promoted_piece = get_game_piece(get_side(game_piece), promotion_list[state.promotion_index]);
                set_piece_mesh(&piece_manager, state.selected_piece, promoted_piece);
            }
        }
        break;

        case StatePhase::ai_think:
        {
            if (searcher.search_state == SearchState::idle)
            {
                searcher.search_state = SearchState::working;
                ASSERT(up_semaphore(searcher.semaphore, 1));
            }
            else if (searcher.search_state == SearchState::finished)
            {
                searcher.search_state = SearchState::idle;
                GameMove best_move = searcher.best_move;
                Square square_from = get_from(best_move);
                state.selected_piece = piece_manager.piece_mapping[square_from];
                ASSERT(state.selected_piece);
                start_animation(&piece_manager, state.selected_piece, &game_state, best_move);
                state.phase = StatePhase::ai_animate;
                state.executing_move = best_move;
            }
        }
        break;

        case StatePhase::ai_animate:
        {
            ASSERT(state.selected_piece);
            if (state.selected_piece->animation_type == AnimationType::none)
            {
                GameMoveTypeEnum move_type = get_move_type(state.executing_move);
                // NOTE: Start promotion
                if (move_type == GameMoveType::promotion)
                {
                    Int promotion_index = get_promotion_index(state.executing_move);
                    GameSideEnum side = get_side(state.executing_move);
                    GamePiece promoted_piece = get_game_piece(side, promotion_list[promotion_index]);
                    set_piece_mesh(&piece_manager, state.selected_piece, promoted_piece);
                }

                // NOTE: Record game move
                record_game_move_with_history(&game_state, state.executing_move);
                record_move(&piece_manager, state.executing_move);
                state.phase = StatePhase::select;
                state.selected_piece = null;

                check_end(&state, &game_state);
            }
        }
        break;

        case StatePhase::menu_blur:
        {
            state.blur_times = MIN(state.blur_times + 1, MAX_BLUR_TIMES);
            if (state.blur_times == MAX_BLUR_TIMES)
            {
                state.phase = StatePhase::menu;
            }
        }
        break;

        case StatePhase::menu_unblur:
        {
            state.blur_times = MAX(state.blur_times - 1, 0);
            if (state.blur_times == 0)
            {
                if (state.menu_game_end == GameEnd::none)
                {
                    if (state.menu_state.selected_player != game_state.player_side)
                    {
                        game_state = get_initial_game_state(asset_store.bit_board_table, state.menu_state.selected_player);
                        fill_piece_manager_initial_state(&piece_manager, &game_state);
                        camera = get_scene_camera(game_state.player_side);
                        reset_state(&state, game_state.player_side);
                    }
                    else
                    {
                        state.phase = state.phase_before_menu;
                    }
                }
                else
                {
                    game_state = get_initial_game_state(asset_store.bit_board_table, game_state.player_side);
                    fill_piece_manager_initial_state(&piece_manager, &game_state);
                    reset_state(&state, game_state.player_side);
                }
            }
        }
        break;

        case StatePhase::menu:
        {
            if (state.menu_game_end == GameEnd::none)
            {
                Vec2 mouse_pos = {(Real)state.mouse_x / window_width * 2 - 1, (Real)state.mouse_y / window_height * 2 - 1};
                state.menu_state.hovered_player = GameSide::count;
                for (GameSideEnum side = 0; side < GameSide::count; side++)
                {
                    Vec2 start_pos = menu_layout.player_pos[side];
                    Vec2 end_pos = menu_layout.player_end_pos[side];

                    if (mouse_pos.x > start_pos.x && mouse_pos.y > start_pos.y && mouse_pos.x < end_pos.x && mouse_pos.y < end_pos.y)
                    {
                        state.menu_state.hovered_player = side;
                        if (input.click_left)
                        {
                            state.menu_state.selected_player = side;
                        }
                    }
                }
            }
        }
        break;

        default:
        {
            ASSERT(false);
        }
        break;
        }

        state.last_hover_square = hover_square;

        DebugUIDrawState debug_ui_draw_state = {};
        DebugMoveDrawState debug_move_draw_state = {};
        if (input.keydown_g)
        {
            state.show_debug = !state.show_debug;
        }
        if (state.show_debug)
        {
            // NOTE: Debug camera
            if (!in_menu(state.phase))
            {
                if (input.keydown_z)
                {
                    camera = get_scene_camera(game_state.player_side);
                }
                else
                {
                    if (input.keydown_d)
                    {
                        state.moving_x_pos = true;
                    }
                    else if (input.keyup_d)
                    {
                        state.moving_x_pos = false;
                    }
                    if (input.keydown_a)
                    {
                        state.moving_x_neg = true;
                    }
                    else if (input.keyup_a)
                    {
                        state.moving_x_neg = false;
                    }
                    if (input.keydown_q)
                    {
                        state.moving_y_pos = true;
                    }
                    else if (input.keyup_q)
                    {
                        state.moving_y_pos = false;
                    }
                    if (input.keydown_e)
                    {
                        state.moving_y_neg = true;
                    }
                    else if (input.keyup_e)
                    {
                        state.moving_y_neg = false;
                    }
                    if (input.keydown_w)
                    {
                        state.moving_z_pos = true;
                    }
                    else if (input.keyup_w)
                    {
                        state.moving_z_pos = false;
                    }
                    if (input.keydown_s)
                    {
                        state.moving_z_neg = true;
                    }
                    else if (input.keyup_s)
                    {
                        state.moving_z_neg = false;
                    }

                    if (input.keydown_i)
                    {
                        state.rotating_x_pos = true;
                    }
                    else if (input.keyup_i)
                    {
                        state.rotating_x_pos = false;
                    }
                    if (input.keydown_k)
                    {
                        state.rotating_x_neg = true;
                    }
                    else if (input.keyup_k)
                    {
                        state.rotating_x_neg = false;
                    }
                    if (input.keydown_l)
                    {
                        state.rotating_y_pos = true;
                    }
                    else if (input.keyup_l)
                    {
                        state.rotating_y_pos = false;
                    }
                    if (input.keydown_j)
                    {
                        state.rotating_y_neg = true;
                    }
                    else if (input.keyup_j)
                    {
                        state.rotating_y_neg = false;
                    }
                    if (input.keydown_o)
                    {
                        state.rotating_z_pos = true;
                    }
                    else if (input.keyup_o)
                    {
                        state.rotating_z_pos = false;
                    }
                    if (input.keydown_u)
                    {
                        state.rotating_z_neg = true;
                    }
                    else if (input.keyup_u)
                    {
                        state.rotating_z_neg = false;
                    }

                    Vec3 camera_x = rotate(camera.rot, get_basis_x());
                    Vec3 camera_y = rotate(camera.rot, get_basis_y());
                    Vec3 camera_z = rotate(camera.rot, get_basis_z());
                    Real speed = 8;
                    if (state.moving_x_pos)
                    {
                        camera.pos = camera.pos + speed * camera_x;
                    }
                    if (state.moving_x_neg)
                    {
                        camera.pos = camera.pos - speed * camera_x;
                    }
                    if (state.moving_y_pos)
                    {
                        camera.pos = camera.pos + speed * camera_y;
                    }
                    if (state.moving_y_neg)
                    {
                        camera.pos = camera.pos - speed * camera_y;
                    }
                    if (state.moving_z_pos)
                    {
                        camera.pos = camera.pos + speed * camera_z;
                    }
                    if (state.moving_z_neg)
                    {
                        camera.pos = camera.pos - speed * camera_z;
                    }

                    Real rotating_speed = degree_to_radian(0.5);
                    Quaternion local_rot = get_identity_quaternion();
                    if (state.rotating_x_pos)
                    {
                        local_rot = get_rotation_quaternion(get_basis_x(), rotating_speed) * local_rot;
                    }
                    if (state.rotating_x_neg)
                    {
                        local_rot = get_rotation_quaternion(get_basis_x(), -rotating_speed) * local_rot;
                    }
                    if (state.rotating_y_pos)
                    {
                        local_rot = get_rotation_quaternion(get_basis_y(), rotating_speed) * local_rot;
                    }
                    if (state.rotating_y_neg)
                    {
                        local_rot = get_rotation_quaternion(get_basis_y(), -rotating_speed) * local_rot;
                    }
                    if (state.rotating_z_pos)
                    {
                        local_rot = get_rotation_quaternion(get_basis_z(), rotating_speed) * local_rot;
                    }
                    if (state.rotating_z_neg)
                    {
                        local_rot = get_rotation_quaternion(get_basis_z(), -rotating_speed) * local_rot;
                    }
                    camera.rot = camera.rot * local_rot;
                }
            }

            // NOTE: Debug UI
            Vec2 debug_ui_start_pos = {-0.95, -0.95};
            debug_ui_draw_state = create_debug_ui_draw_state(&asset_store.debug_font, window_width, window_height, &debug_ui_vertex_buffer, debug_ui_start_pos);

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
            if (state.selected_piece)
            {
                GamePiece game_piece = game_state.board[state.selected_piece->square];
                Str piece_name = get_game_piece_name(game_piece);
                debug_ui_draw_str(&debug_ui_draw_state, piece_name);
            }
            else
            {
                debug_ui_draw_str(&debug_ui_draw_state, str("<no>"));
            }
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("selected square: "));
            if (hover_square != NO_SQUARE)
            {
                Square row = get_row(hover_square);
                Square column = get_column(hover_square);
                debug_ui_draw_int(&debug_ui_draw_state, hover_square);
                debug_ui_draw_str(&debug_ui_draw_state, str(" ("));
                debug_ui_draw_int(&debug_ui_draw_state, row);
                debug_ui_draw_str(&debug_ui_draw_state, str(", "));
                debug_ui_draw_int(&debug_ui_draw_state, column);
                debug_ui_draw_str(&debug_ui_draw_state, str(")"));
            }
            else
            {
                debug_ui_draw_str(&debug_ui_draw_state, str("<no>"));
            }
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("current side: "));
            debug_ui_draw_str(&debug_ui_draw_state, game_state.current_side == GameSide::white ? str("white") : str("black"));
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("castling white: "));
            debug_ui_draw_str(&debug_ui_draw_state, game_state.castling & get_castling_mask(GameSide::white, GameCastlingMask::queen_side) ? str("ooo") : str(""));
            debug_ui_draw_str(&debug_ui_draw_state, str(" "));
            debug_ui_draw_str(&debug_ui_draw_state, game_state.castling & get_castling_mask(GameSide::white, GameCastlingMask::king_side) ? str("oo") : str(""));
            debug_ui_draw_newline(&debug_ui_draw_state);
            debug_ui_draw_str(&debug_ui_draw_state, str("castling black: "));
            debug_ui_draw_str(&debug_ui_draw_state, game_state.castling & get_castling_mask(GameSide::black, GameCastlingMask::queen_side) ? str("ooo") : str(""));
            debug_ui_draw_str(&debug_ui_draw_state, str(" "));
            debug_ui_draw_str(&debug_ui_draw_state, game_state.castling & get_castling_mask(GameSide::black, GameCastlingMask::king_side) ? str("oo") : str(""));

            // NOTE: Debug move
            if (state.phase == StatePhase::execute)
            {
                debug_move_draw_state.vertex_buffer = &debug_move_vertex_buffer;
                debug_move_draw_state.count = 0;
                for (Square square = 0; square < 64; square++)
                {
                    if (all_moves & bit_square(square))
                    {
                        GameMove move = get_game_move(&game_state, state.selected_piece->square, square);
                        if (is_move_legal(&game_state, move))
                        {
                            debug_move_draw(&debug_move_draw_state, square, Vec3{0, 1, 0});
                        }
                        else
                        {
                            debug_move_draw(&debug_move_draw_state, square, Vec3{1, 0, 0});
                        }
                    }
                }
            }
        }

        Int menu_vertex_count = 0;
        if (in_menu(state.phase))
        {
            if (state.menu_game_end == GameEnd::none)
            {
                menu_vertex_count = draw_menu(asset_store.menu_fonts, &state.menu_state, (Real)state.blur_times / MAX_BLUR_TIMES, window_width, window_height, &menu_vertex_buffer);
            }
            else
            {
                menu_vertex_count = draw_game_end(asset_store.menu_fonts, state.menu_game_end, (Real)state.blur_times / MAX_BLUR_TIMES, window_width, window_height, &menu_vertex_buffer);
            }
        }
        else
        {
            calculate_scene_uniform_data(&camera, window_width, window_height, scene_uniform_data);
            for (Int piece_i = 0; piece_i < ENTITY_PIECE_COUNT; piece_i++)
            {
                Piece *piece = &piece_manager.pieces[piece_i];
                if (piece->square != NO_SQUARE)
                {
                    update_animation(piece, frame_time);
                    calculate_entity_uniform_data(piece, get_piece_uniform_data(&device, &scene_uniform_buffer, piece_i));
                }
            }
            if (show_ghost_piece)
            {
                update_animation(&ghost_piece, frame_time);
                calculate_entity_uniform_data(&ghost_piece, get_ghost_piece_uniform_data(&device, &scene_uniform_buffer));
            }
        }

        ASSERT(render_vulkan_frame(&device,
                                   &scene_pipeline, &scene_frame, &scene_uniform_buffer, &board, &piece_manager, show_ghost_piece ? &ghost_piece : null,
                                   &shadow_pipeline, &shadow_frame,
                                   state.show_debug,
                                   &debug_ui_pipeline, &debug_ui_frame, &debug_ui_vertex_buffer, debug_ui_draw_state.character_count,
                                   &debug_collision_pipeline, &debug_collision_frame, &debug_collision_vertex_buffer,
                                   &debug_move_pipeline, &debug_move_frame, &debug_move_vertex_buffer, debug_move_draw_state.count,
                                   &blur_pipeline, &blur_frame, state.blur_times, &menu_pipeline, &menu_frame, &menu_vertex_buffer, menu_vertex_count));

        if (sound_renderer.sound)
        {
            Sound *sound = sound_renderer.sound;
            Int output_frame_count = frame_time * sound->frame_rate * 2;
            Int available_frame_count = MIN(output_frame_count, sound->frame_count - sound_renderer.frame_pos);
            UInt8 *audio_buffer;
            HRESULT result = sound_renderer.audio_render_client->GetBuffer(available_frame_count, &audio_buffer);
            ASSERT(SUCCEEDED(result));

            UInt8 *audio_data = asset_store.sound_error.data + sound_renderer.frame_pos * sound->channel_count * sound->sample_byte_count;
            for (Int frame_i = 0; frame_i < available_frame_count; frame_i++)
            {
                Real32 sample = (Real) * (Int16 *)audio_data / 0x7fff;
                *(Real32 *)audio_buffer = sample;
                audio_buffer += 4;
                audio_data += 2;

                sample = (Real) * (Int16 *)audio_data / 0x7fff;
                *(UInt32 *)audio_buffer = sample;
                audio_buffer += 4;
                audio_data += 2;
            }

            result = sound_renderer.audio_render_client->ReleaseBuffer(available_frame_count, 0);
            ASSERT(SUCCEEDED(result));

            if (!sound_renderer.frame_pos)
            {
                result = sound_renderer.audio_client->Start();
                ASSERT(SUCCEEDED(result));
            }
            sound_renderer.frame_pos += available_frame_count;
            if (sound_renderer.frame_pos == sound->frame_count)
            {
                sound_renderer.sound = null;
                sound_renderer.audio_client->Stop();
                sound_renderer.audio_client->Reset();
            }
        }

        UInt64 current_timestamp = get_current_timestamp();
        Real64 elapsed_time = get_elapsed_time(current_timestamp - last_timestamp);

        // char string[256];
        // sprintf(string, "elapsed time = %f, fps = %d\n", elapsed_time, (Int)(1.0 / elapsed_time));
        // OutputDebugStringA(string);

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
