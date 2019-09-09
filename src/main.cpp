#include "../lib/util.hpp"
#include "../lib/vulkan.hpp"
#include "math.cpp"
#include "asset.cpp"
#include "window.cpp"
#include "scene.cpp"
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

Bool read_font(CStr filename, OUT Font *font)
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

struct Camera
{
    Vec3 pos;
    Mat4 rotation;
};

Void add_entity(Array<Entity> *entities, Str name, Vec3 pos, Mat4 rotation, Mesh *mesh)
{
    Entity *entity = entities->push();
    entity->name = name;
    entity->pos = pos;
    entity->rotation = rotation;
    entity->mesh = mesh;
}

Void debug_callback(Str message)
{
    OutputDebugStringA((LPCSTR)message.data);
    OutputDebugStringA("\n");
}

Bool render_vulkan_frame(VulkanDevice *device,
                         VulkanPipeline *scene_pipeline, SceneFrame *scene_frame, VulkanBuffer *scene_uniform_buffer, Array<Entity> *entities,
                         VulkanPipeline *debug_ui_pipeline, DebugUIFrame *debug_ui_frame, VulkanBuffer *debug_ui_vertex_buffer, Int debug_ui_character_count,
                         VulkanPipeline *debug_collision_pipeline, DebugCollisionFrame *debug_collision_frame, VulkanBuffer *debug_collision_vertex_buffer, VulkanBuffer *debug_collision_uniform_buffer)
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
    scene_uniform_buffer_copy.size = sizeof(SceneUniformData);
    vkCmdCopyBuffer(scene_frame->command_buffer, scene_uniform_buffer->handle, scene_frame->uniform_buffer.handle, 1, &scene_uniform_buffer_copy);

    VkBufferCopy debug_ui_vertex_buffer_copy = {};
    debug_ui_vertex_buffer_copy.srcOffset = 0;
    debug_ui_vertex_buffer_copy.dstOffset = 0;
    debug_ui_vertex_buffer_copy.size = sizeof(DebugUIVertex) * debug_ui_character_count * 6;
    vkCmdCopyBuffer(scene_frame->command_buffer, debug_ui_vertex_buffer->handle, debug_ui_frame->vertex_buffer.handle, 1, &debug_ui_vertex_buffer_copy);

    VkBufferMemoryBarrier scene_uniform_buffer_memory_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    scene_uniform_buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    scene_uniform_buffer_memory_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
    scene_uniform_buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    scene_uniform_buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    scene_uniform_buffer_memory_barrier.buffer = scene_frame->uniform_buffer.handle;
    scene_uniform_buffer_memory_barrier.offset = 0;
    scene_uniform_buffer_memory_barrier.size = sizeof(SceneUniformData);
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, null, 1, &scene_uniform_buffer_memory_barrier, 0, null);

    VkBufferMemoryBarrier debug_ui_vertex_buffer_memory_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    debug_ui_vertex_buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    debug_ui_vertex_buffer_memory_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    debug_ui_vertex_buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    debug_ui_vertex_buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    debug_ui_vertex_buffer_memory_barrier.buffer = debug_ui_frame->vertex_buffer.handle;
    debug_ui_vertex_buffer_memory_barrier.offset = 0;
    debug_ui_vertex_buffer_memory_barrier.size = sizeof(DebugUIVertex) * debug_ui_character_count * 6;
    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, null, 1, &debug_ui_vertex_buffer_memory_barrier, 0, null);

    VkImageMemoryBarrier depth_image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
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

    VkImageMemoryBarrier color_image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
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

    // NOTE: Scene
    VkClearValue clear_colors[2] = {{0.7, 0.7, 0.7, 0.7},
                                    {1.0, 0}};
    VkRenderPassBeginInfo scene_render_pass_begin_info = {};
    scene_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    scene_render_pass_begin_info.renderPass = scene_pipeline->render_pass;
    scene_render_pass_begin_info.framebuffer = scene_frame->frame_buffers[image_index];
    scene_render_pass_begin_info.renderArea.offset = {0, 0};
    scene_render_pass_begin_info.renderArea.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};
    scene_render_pass_begin_info.clearValueCount = 2;
    scene_render_pass_begin_info.pClearValues = clear_colors;

    vkCmdBeginRenderPass(scene_frame->command_buffer, &scene_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->handle);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &scene_frame->vertex_buffer.handle, &offset);
    vkCmdBindIndexBuffer(scene_frame->command_buffer, scene_frame->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    Int index_offset = 0;
    Int vertex_offset = 0;
    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 0, 1, &scene_frame->scene_descriptor_set, 0, null);
    for (Int entity_i = 0; entity_i < entities->count; entity_i++)
    {
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 1, 1, &scene_frame->entity_descriptor_sets[entity_i], 0, null);
        vkCmdDrawIndexed(scene_frame->command_buffer, entities->data[entity_i].mesh->index_count, 1, index_offset, vertex_offset, 0);
        index_offset += entities->data[entity_i].mesh->index_count;
        vertex_offset += entities->data[entity_i].mesh->vertex_count;
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

        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_collision_pipeline->layout, 0, 1, &debug_collision_frame->uniform_descriptor_set, 0, null);

        vertex_offset = 0;
        for (Int entity_i = 1; entity_i < entities->count; entity_i++)
        {
            vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_collision_pipeline->layout, 1, 1, &scene_frame->entity_descriptor_sets[entity_i], 0, null);

            vkCmdDraw(scene_frame->command_buffer, 12 * 2, 1, vertex_offset, 0);
            vertex_offset += 12 * 2;
        }

        vkCmdEndRenderPass(scene_frame->command_buffer);
    }

    // NOTE: Debug UI
    if (debug_ui_character_count > 0)
    {
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

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    Int window_width = 800;
    Int window_height = 600;

    Mesh board_mesh;
    ASSERT(read_mesh("../asset/board.asset", &board_mesh));

    Mesh black_king_mesh;
    ASSERT(read_mesh("../asset/king_black.asset", &black_king_mesh));
    Mesh black_queen_mesh;
    ASSERT(read_mesh("../asset/queen_black.asset", &black_queen_mesh));
    Mesh black_bishop_mesh;
    ASSERT(read_mesh("../asset/bishop_black.asset", &black_bishop_mesh));
    Mesh black_knight_mesh;
    ASSERT(read_mesh("../asset/knight_black.asset", &black_knight_mesh));
    Mesh black_rook_mesh;
    ASSERT(read_mesh("../asset/rook_black.asset", &black_rook_mesh));
    Mesh black_pawn_mesh;
    ASSERT(read_mesh("../asset/pawn_black.asset", &black_pawn_mesh));

    Mesh white_king_mesh;
    ASSERT(read_mesh("../asset/king_white.asset", &white_king_mesh));
    Mesh white_queen_mesh;
    ASSERT(read_mesh("../asset/queen_white.asset", &white_queen_mesh));
    Mesh white_bishop_mesh;
    ASSERT(read_mesh("../asset/bishop_white.asset", &white_bishop_mesh));
    Mesh white_knight_mesh;
    ASSERT(read_mesh("../asset/knight_white.asset", &white_knight_mesh));
    Mesh white_rook_mesh;
    ASSERT(read_mesh("../asset/rook_white.asset", &white_rook_mesh));
    Mesh white_pawn_mesh;
    ASSERT(read_mesh("../asset/pawn_white.asset", &white_pawn_mesh));

    Font debug_font;
    ASSERT(read_font("../asset/debug_font.asset", &debug_font));

    Mat4 black_inital_model = get_identity_matrix();
    Mat4 white_inital_model = get_rotation_matrix_y(PI);

    Array<Entity> entities = create_array<Entity>();
    add_entity(&entities, str("board"), {0, 0, 0}, black_inital_model, &board_mesh);

    add_entity(&entities, str("black_rook1"), {0, 0, 0}, black_inital_model, &black_rook_mesh);
    add_entity(&entities, str("black_knight1"), {-100, 0, 0}, black_inital_model, &black_knight_mesh);
    add_entity(&entities, str("black_bishop1"), {-200, 0, 0}, black_inital_model, &black_bishop_mesh);
    add_entity(&entities, str("black_king"), {-300, 0, 0}, black_inital_model, &black_king_mesh);
    add_entity(&entities, str("black_queen"), {-400, 0, 0}, black_inital_model, &black_queen_mesh);
    add_entity(&entities, str("black_bishop2"), {-500, 0, 0}, black_inital_model, &black_bishop_mesh);
    add_entity(&entities, str("black_knight2"), {-600, 0, 0}, black_inital_model, &black_knight_mesh);
    add_entity(&entities, str("black_rook2"), {-700, 0, 0}, black_inital_model, &black_rook_mesh);
    for (Int entity_i = 0; entity_i < 8; entity_i++)
    {
        UInt8 number_suffix_data[2];
        number_suffix_data[0] = '0' + entity_i;
        number_suffix_data[1] = '\0';
        Str number_suffix;
        number_suffix.count = 1;
        number_suffix.data = number_suffix_data;
        add_entity(&entities, concat_str(str("black_pawn"), number_suffix), {-entity_i * 100.0f, 0, 100}, get_identity_matrix(), &black_pawn_mesh);
    }

    add_entity(&entities, str("white_rook1"), {0, 0, 700}, white_inital_model, &white_rook_mesh);
    add_entity(&entities, str("white_knight1"), {-100, 0, 700}, white_inital_model, &white_knight_mesh);
    add_entity(&entities, str("white_bishop1"), {-200, 0, 700}, white_inital_model, &white_bishop_mesh);
    add_entity(&entities, str("white_king"), {-300, 0, 700}, white_inital_model, &white_king_mesh);
    add_entity(&entities, str("white_queen"), {-400, 0, 700}, white_inital_model, &white_queen_mesh);
    add_entity(&entities, str("white_bishop2"), {-500, 0, 700}, white_inital_model, &white_bishop_mesh);
    add_entity(&entities, str("white_knight2"), {-600, 0, 700}, white_inital_model, &white_knight_mesh);
    add_entity(&entities, str("white_rook2"), {-700, 0, 700}, white_inital_model, &white_rook_mesh);
    for (Int entity_i = 0; entity_i < 8; entity_i++)
    {
        UInt8 number_suffix_data[2];
        number_suffix_data[0] = '0' + entity_i;
        number_suffix_data[1] = '\0';
        Str number_suffix;
        number_suffix.count = 1;
        number_suffix.data = number_suffix_data;
        add_entity(&entities, concat_str(str("white_pawn"), number_suffix), {-entity_i * 100.0f, 0, 600}, white_inital_model, &white_pawn_mesh);
    }

    for (Int entity_i = 1; entity_i < entities.count; entity_i++)
    {
        Vec3 min = {+10000, +10000, +10000};
        Vec3 max = {-10000, -10000, -10000};

        Entity *entity = &entities[entity_i];
        for (Int vertex_i = 0; vertex_i < entity->mesh->vertex_count; vertex_i++)
        {
            Vec3 pos = entity->mesh->vertices_data[vertex_i].pos;
            for (Int i = 0; i < 3; i++)
            {
                min[i] = MIN(min[i], pos[i]);
                max[i] = MAX(max[i], pos[i]);
            }
        }
        min.y = 0;

        entity->collision_box.center = 0.5 * (max + min);
        entity->collision_box.radius = 0.5 * (max - min);
    }

    Camera camera;
    camera.pos = {-350, 1600, -450};
    camera.rotation = get_rotation_matrix_x(degree_to_radian(65)) * get_rotation_matrix_z(PI);

    SceneUniformData scene_uniform_data;
    scene_uniform_data.view = get_view_matrix(camera.pos, vec3(camera.rotation.z), -vec3(camera.rotation.y));
    scene_uniform_data.normal_view = get_normal_view_matrix(camera.pos, vec3(camera.rotation.z), -vec3(camera.rotation.y));
    scene_uniform_data.projection = get_perspective_matrix(degree_to_radian(30), (Real)window_width / (Real)window_height, 10, 10000);
    scene_uniform_data.light_dir[0] = {1, -1, 1};
    scene_uniform_data.light_dir[1] = {1, -1, -1};
    scene_uniform_data.light_dir[2] = {-1, -1, -1};
    scene_uniform_data.light_dir[3] = {-1, -1, 1};

    DebugCollisionUniformData debug_collision_uniform_data;
    debug_collision_uniform_data.view = scene_uniform_data.view;
    debug_collision_uniform_data.projection = scene_uniform_data.projection;

    Mat4 inverse_projection;
    ASSERT(inverse(scene_uniform_data.projection, &inverse_projection));

    Window window = create_window(str("Chess"), window_width, window_height, 50, 50);
    ASSERT(window);

    VulkanDevice device;
    ASSERT(create_device(window, debug_callback, &device));
    ASSERT(create_swapchain(&device, 800, 600, 3, VK_PRESENT_MODE_MAILBOX_KHR));

    VulkanPipeline scene_pipeline;
    ASSERT(create_scene_pipeline(&device, &scene_pipeline));

    VulkanPipeline debug_ui_pipeline;
    ASSERT(create_debug_ui_pipeline(&device, &debug_ui_pipeline));

    VulkanPipeline debug_collision_pipeline;
    ASSERT(create_debug_collision_pipeline(&device, &debug_collision_pipeline));

    SceneFrame scene_frame;
    VulkanBuffer scene_vertex_buffer;
    VulkanBuffer scene_index_buffer;
    VulkanBuffer scene_uniform_buffer;
    ASSERT(create_scene_frame(&device, &scene_pipeline, &entities, &scene_frame, &scene_vertex_buffer, &scene_index_buffer, &scene_uniform_buffer));

    DebugUIFrame debug_ui_frame;
    VulkanBuffer debug_ui_vertex_buffer;
    ASSERT(create_debug_ui_frame(&device, &debug_ui_pipeline, &debug_font, &debug_ui_frame, &debug_ui_vertex_buffer));

    DebugCollisionFrame debug_collision_frame;
    VulkanBuffer debug_collision_vertex_buffer;
    VulkanBuffer debug_collision_uniform_buffer;
    ASSERT(create_debug_collision_frame(&device, &debug_collision_pipeline, &entities, &debug_collision_frame, &debug_collision_vertex_buffer, &debug_collision_uniform_buffer));

    Int current_vertex_data_offset = 0;
    Int current_index_data_offset = 0;
    Int current_uniform_data_offset = 0;
    memcpy(scene_uniform_buffer.data, &scene_uniform_data, sizeof(scene_uniform_data));
    current_uniform_data_offset += sizeof(scene_uniform_data);
    for (Int entity_i = 0; entity_i < entities.count; entity_i++)
    {
        Mesh *mesh = entities[entity_i].mesh;
        Int current_vertex_data_length = mesh->vertex_count * sizeof(mesh->vertices_data[0]);
        Int current_index_data_length = mesh->index_count * sizeof(mesh->indices_data[0]);
        memcpy(scene_vertex_buffer.data + current_vertex_data_offset, entities[entity_i].mesh->vertices_data, current_vertex_data_length);
        memcpy(scene_index_buffer.data + current_index_data_offset, entities[entity_i].mesh->indices_data, current_index_data_length);

        EntityUniformData *entity_uniform_data = (EntityUniformData *)(scene_uniform_buffer.data + current_uniform_data_offset);
        entity_uniform_data->world = get_translate_matrix(entities[entity_i].pos.x, entities[entity_i].pos.y, entities[entity_i].pos.z) * entities[entity_i].rotation;
        entity_uniform_data->normal_world = entities[entity_i].rotation;

        current_vertex_data_offset += current_vertex_data_length;
        current_index_data_offset += current_index_data_length;
        current_uniform_data_offset += sizeof(EntityUniformData);
    }

    ASSERT(upload_buffer(&device, &scene_vertex_buffer, &scene_frame.vertex_buffer));
    ASSERT(upload_buffer(&device, &scene_index_buffer, &scene_frame.index_buffer));
    ASSERT(upload_buffer(&device, &scene_uniform_buffer, &scene_frame.uniform_buffer));

    DebugCollisionVertex *vertex = (DebugCollisionVertex *)debug_collision_vertex_buffer.data;
    for (Int entity_i = 1; entity_i < entities.count; entity_i++)
    {
        CollisionBox *collision_box = &entities[entity_i].collision_box;

        {
            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;
        }

        {
            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;
        }

        {
            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z - collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;
        }

        {
            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x + collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y + collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;

            vertex->pos.x = collision_box->center.x - collision_box->radius.x;
            vertex->pos.y = collision_box->center.y - collision_box->radius.y;
            vertex->pos.z = collision_box->center.z + collision_box->radius.z;
            vertex->color = {1, 0, 0};
            vertex++;
        }
    }
    memcpy(debug_collision_uniform_buffer.data, &debug_collision_uniform_data, sizeof(debug_collision_uniform_data));

    ASSERT(upload_buffer(&device, &debug_collision_vertex_buffer, &debug_collision_frame.vertex_buffer));
    ASSERT(upload_buffer(&device, &debug_collision_uniform_buffer, &debug_collision_frame.uniform_buffer));

    show_window(window);

    Bool moving_x_pos = false;
    Bool moving_x_neg = false;
    Bool moving_y_pos = false;
    Bool moving_y_neg = false;
    Bool moving_z_pos = false;
    Bool moving_z_neg = false;

    Bool rotating_x_pos = false;
    Bool rotating_x_neg = false;
    Bool rotating_y_pos = false;
    Bool rotating_y_neg = false;
    Bool rotating_z_pos = false;
    Bool rotating_z_neg = false;

    Int mouse_x;
    Int mouse_y;

    Bool is_running = true;
    Bool show_debug_ui = false;
    while (is_running)
    {
        WindowMessage message;
        while (get_window_message(window, &message))
        {
            if (message.type == WindowMessageType::close)
            {
                is_running = false;
                break;
            }
            else if (message.type == WindowMessageType::key_down)
            {
                WindowMessageKeyCode key_code = message.key_down_data.key_code;

                if (key_code == WindowMessageKeyCode::key_d)
                {
                    moving_x_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_a)
                {
                    moving_x_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_q)
                {
                    moving_y_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_e)
                {
                    moving_y_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_w)
                {
                    moving_z_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_s)
                {
                    moving_z_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_i)
                {
                    rotating_x_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_k)
                {
                    rotating_x_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_l)
                {
                    rotating_y_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_j)
                {
                    rotating_y_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_o)
                {
                    rotating_z_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_u)
                {
                    rotating_z_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_g)
                {
                    show_debug_ui = !show_debug_ui;
                }
            }
            else if (message.type == WindowMessageType::key_up)
            {
                WindowMessageKeyCode key_code = message.key_down_data.key_code;

                if (key_code == WindowMessageKeyCode::key_d)
                {
                    moving_x_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_a)
                {
                    moving_x_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_q)
                {
                    moving_y_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_e)
                {
                    moving_y_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_w)
                {
                    moving_z_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_s)
                {
                    moving_z_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_i)
                {
                    rotating_x_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_k)
                {
                    rotating_x_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_l)
                {
                    rotating_y_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_j)
                {
                    rotating_y_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_o)
                {
                    rotating_z_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_u)
                {
                    rotating_z_neg = false;
                }
            }
            else if (message.type == WindowMessageType::mouse_down)
            {
                // if (message.mouse_down_data.button_type == WindowMessageMouseButtonType::left)
                // {
                //     mouse_x = message.mouse_down_data.x;
                //     mouse_y = message.mouse_down_data.y;
                // }
            }
            else if (message.type == WindowMessageType::mouse_move)
            {
                mouse_x = message.mouse_move_data.x;
                mouse_y = message.mouse_move_data.y;
            }
        }

        Real speed = 3;
        if (moving_x_pos)
        {
            camera.pos = camera.pos + speed * vec3(camera.rotation.x);
        }
        else if (moving_x_neg)
        {
            camera.pos = camera.pos - speed * vec3(camera.rotation.x);
        }
        else if (moving_y_pos)
        {
            camera.pos = camera.pos + speed * vec3(camera.rotation.y);
        }
        else if (moving_y_neg)
        {
            camera.pos = camera.pos - speed * vec3(camera.rotation.y);
        }
        else if (moving_z_pos)
        {
            camera.pos = camera.pos + speed * vec3(camera.rotation.z);
        }
        else if (moving_z_neg)
        {
            camera.pos = camera.pos - speed * vec3(camera.rotation.z);
        }

        Real rotating_speed = degree_to_radian(0.2);
        Mat4 local_transform = get_identity_matrix();
        if (rotating_x_pos)
        {
            local_transform = get_rotation_matrix_x(rotating_speed) * local_transform;
        }
        else if (rotating_x_neg)
        {
            local_transform = get_rotation_matrix_x(-rotating_speed) * local_transform;
        }
        else if (rotating_y_pos)
        {
            local_transform = get_rotation_matrix_y(rotating_speed) * local_transform;
        }
        else if (rotating_y_neg)
        {
            local_transform = get_rotation_matrix_y(-rotating_speed) * local_transform;
        }
        else if (rotating_z_pos)
        {
            local_transform = get_rotation_matrix_z(rotating_speed) * local_transform;
        }
        else if (rotating_z_neg)
        {
            local_transform = get_rotation_matrix_z(-rotating_speed) * local_transform;
        }

        camera.rotation = camera.rotation * local_transform;

        scene_uniform_data.view = get_view_matrix(camera.pos, vec3(camera.rotation.z), -vec3(camera.rotation.y));
    
        memcpy(scene_uniform_buffer.data, &scene_uniform_data, sizeof(scene_uniform_data));

        Mat4 inverse_projection;
        ASSERT(inverse(scene_uniform_data.projection, &inverse_projection));

        Mat4 inverse_view;
        ASSERT(inverse(scene_uniform_data.view, &inverse_view));

        Vec4 mouse_pos_clip = {(Real)mouse_x * 2 / window_width - 1, (Real)mouse_y * 2 / window_height - 1, 0, 1};
        Vec3 mouse_pos = vec3(perspective_divide(inverse_view * inverse_projection * mouse_pos_clip));
        Vec3 dir = normalize(mouse_pos - camera.pos);
        Ray ray;
        ray.pos = camera.pos;
        ray.dir = dir;
    
        Real min_dist = 10000000;
        Int selected_entity_index = -1;
        for (Int entity_i = 1; entity_i < entities.count; entity_i++)
        {
            Entity *entity = &entities[entity_i];
            CollisionBox collision_box;
            collision_box.center = entity->collision_box.center + entity->pos;
            collision_box.radius = entity->collision_box.radius;

            Real dist = check_collision(&ray, &collision_box);
            if (dist > 0 && dist < min_dist)
            {
                selected_entity_index = entity_i;
            }
        }

        debug_collision_uniform_data.view = scene_uniform_data.view;
        memcpy(debug_collision_uniform_buffer.data, &debug_collision_uniform_data, sizeof(debug_collision_uniform_data));

        DebugUIDrawState debug_ui_draw_state;
        debug_ui_draw_state.character_count = 0;
        if (show_debug_ui)
        {
            Vec2 debug_ui_start_pos = {-0.95, -0.95};
            debug_ui_draw_state = create_debug_ui_draw_state(&debug_font, window_width, window_height, &debug_ui_vertex_buffer, debug_ui_start_pos);

            debug_ui_draw_str(&debug_ui_draw_state, str("camera"));
            debug_ui_draw_newline(&debug_ui_draw_state);
            debug_ui_draw_indent(&debug_ui_draw_state, 1);

            debug_ui_draw_str(&debug_ui_draw_state, str("position: "));
            debug_ui_draw_vec3(&debug_ui_draw_state, camera.pos);
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("rotation x: "));
            debug_ui_draw_vec3(&debug_ui_draw_state, vec3(camera.rotation.x));
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("rotation y: "));
            debug_ui_draw_vec3(&debug_ui_draw_state, vec3(camera.rotation.y));
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("rotation z: "));
            debug_ui_draw_vec3(&debug_ui_draw_state, vec3(camera.rotation.z));
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_indent(&debug_ui_draw_state, -1);

            debug_ui_draw_str(&debug_ui_draw_state, str("ray"));
            debug_ui_draw_newline(&debug_ui_draw_state);
            debug_ui_draw_indent(&debug_ui_draw_state, 1);

            debug_ui_draw_str(&debug_ui_draw_state, str("position: "));
            debug_ui_draw_vec3(&debug_ui_draw_state, ray.pos);
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("dir: "));
            debug_ui_draw_vec3(&debug_ui_draw_state, ray.dir);
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("selected piece: "));
            debug_ui_draw_int(&debug_ui_draw_state, selected_entity_index);
        }

        ASSERT(render_vulkan_frame(&device,
                                   &scene_pipeline, &scene_frame, &scene_uniform_buffer, &entities,
                                   &debug_ui_pipeline, &debug_ui_frame, &debug_ui_vertex_buffer, debug_ui_draw_state.character_count,
                                   &debug_collision_pipeline, &debug_collision_frame, &debug_collision_vertex_buffer, &debug_collision_uniform_buffer));
    }

    return 0;
}

#include "../lib/util.cpp"
#include "../lib/vulkan.cpp"
#include "../lib/os.cpp"
