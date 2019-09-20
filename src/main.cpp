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

Void write_entity_vertex_data(Entity *entity, VulkanBuffer *scene_vertex_buffer, VulkanBuffer *scene_index_buffer, Int *vertex_data_offset, Int *index_data_offset)
{
    Mesh *mesh = entity->mesh;
    Int vertex_data_length = mesh->vertex_count * sizeof(mesh->vertices_data[0]);
    Int index_data_length = mesh->index_count * sizeof(mesh->indices_data[0]);
    memcpy(scene_vertex_buffer->data + *vertex_data_offset, mesh->vertices_data, vertex_data_length);
    memcpy(scene_index_buffer->data + *index_data_offset, mesh->indices_data, index_data_length);

    *vertex_data_offset += vertex_data_length;
    *index_data_offset += index_data_length;
}

Void debug_callback(Str message)
{
    OutputDebugStringA((LPCSTR)message.data);
    OutputDebugStringA("\n");
}

Bool render_vulkan_frame(VulkanDevice *device,
                         VulkanPipeline *scene_pipeline, SceneFrame *scene_frame, VulkanBuffer *scene_uniform_buffer, Board *board, Piece *pieces,
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
    scene_uniform_buffer_memory_barrier.size = scene_uniform_buffer->count;
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

    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 1, 1, &scene_frame->board_descriptor_set, 0, null);
    vkCmdDrawIndexed(scene_frame->command_buffer, board->mesh->index_count, 1, index_offset, vertex_offset, 0);
    index_offset += board->mesh->index_count;
    vertex_offset += board->mesh->vertex_count;

    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->layout, 1, 1, &scene_frame->piece_descriptor_sets[piece_i], 0, null);
        vkCmdDrawIndexed(scene_frame->command_buffer, pieces[piece_i].mesh->index_count, 1, index_offset, vertex_offset, 0);
        index_offset += pieces[piece_i].mesh->index_count;
        vertex_offset += pieces[piece_i].mesh->vertex_count;
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

        vertex_offset = 0;
        for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
        {
            vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_collision_pipeline->layout, 1, 1, &scene_frame->piece_descriptor_sets[piece_i], 0, null);
            vkCmdDraw(scene_frame->command_buffer, 12 * 2, 1, vertex_offset, 0);
            Int line_count = 12;
            vertex_offset += line_count * 2;
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

    GameState game_state = get_initial_game_state();

    Board board;
    board.pos = {0, 0, 0};
    board.rotation = get_rotation_quaternion(get_basis_y(), 0);
    board.scale = {-1, -1, 1};
    board.mesh = &board_mesh;

    // NOTE: Set up board collision box
    for (Int square_i = 0; square_i < BOARD_SQUARE_COUNT; square_i++)
    {
        Int row = get_row(square_i);
        Int column = get_column(square_i);

        CollisionBox *collision_box = &board.collision_box[square_i];
        collision_box->center = {(Real)(column * 100), 0, (Real)(row * 100)};
        collision_box->radius = {50, 0, 50};
    }

    Piece pieces[PIECE_COUNT];
    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        Piece *piece = &pieces[piece_i];
        fill_piece_initial_state(&game_state.pieces[piece_i], piece,
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
                                 &black_pawn_mesh);
    }

    Window window = create_window(str("Chess"), window_width, window_height, 50, 50);
    ASSERT(window);

    VulkanDevice device;
    ASSERT(create_device(window, debug_callback, &device));
    ASSERT(create_swapchain(&device, window_width, window_height, 3, VK_PRESENT_MODE_MAILBOX_KHR));

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
    ASSERT(create_scene_frame(&device, &scene_pipeline, &board, pieces, &scene_frame, &scene_vertex_buffer, &scene_index_buffer, &scene_uniform_buffer));

    DebugUIFrame debug_ui_frame;
    VulkanBuffer debug_ui_vertex_buffer;
    ASSERT(create_debug_ui_frame(&device, &debug_ui_pipeline, &debug_font, &debug_ui_frame, &debug_ui_vertex_buffer));

    DebugCollisionFrame debug_collision_frame;
    VulkanBuffer debug_collision_vertex_buffer;
    ASSERT(create_debug_collision_frame(&device, &debug_collision_pipeline, pieces, &debug_collision_frame, &debug_collision_vertex_buffer));

    Camera camera;
    camera.pos = {350, -1600, -450};
    camera.rotation = get_rotation_quaternion(get_basis_x(), -degree_to_radian(65));

    SceneUniformData *scene_uniform_data = get_scene_uniform_data(&device, &scene_uniform_buffer);
    calculate_scene_uniform_data(&camera, window_width, window_height, scene_uniform_data);
    scene_uniform_data->light_dir[0] = {1, -1, 1};
    scene_uniform_data->light_dir[1] = {1, -1, -1};
    scene_uniform_data->light_dir[2] = {-1, -1, -1};
    scene_uniform_data->light_dir[3] = {-1, -1, 1};

    calculate_entity_uniform_data(&board, get_board_uniform_data(&device, &scene_uniform_buffer));

    Int vertex_data_offset = 0;
    Int index_data_offset = 0;
    write_entity_vertex_data(&board, &scene_vertex_buffer, &scene_index_buffer, &vertex_data_offset, &index_data_offset);
    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        write_entity_vertex_data(&pieces[piece_i], &scene_vertex_buffer, &scene_index_buffer, &vertex_data_offset, &index_data_offset);
    }

    ASSERT(upload_buffer(&device, &scene_vertex_buffer, &scene_frame.vertex_buffer));
    ASSERT(upload_buffer(&device, &scene_index_buffer, &scene_frame.index_buffer));

    DebugCollisionVertex *vertex = (DebugCollisionVertex *)debug_collision_vertex_buffer.data;
    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        CollisionBox *collision_box = &pieces[piece_i].collision_box;
        write_collision_box_vertex_data(collision_box, vertex);

        Int line_count = 12;
        vertex += line_count * 2;
    }

    ASSERT(upload_buffer(&device, &debug_collision_vertex_buffer, &debug_collision_frame.vertex_buffer));

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

    Bool is_running = true;
    Bool show_debug_ui = false;
    while (is_running)
    {
        Int mouse_x = -1;
        Int mouse_y = -1;
        WindowMessageMouseButtonType mouse_button;

        WindowMessage message;
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
            break;

            case WindowMessageType::key_up:
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
            break;

            case WindowMessageType::mouse_down:
            {
                mouse_x = message.mouse_down_data.x;
                mouse_y = message.mouse_down_data.y;
                mouse_button = message.mouse_down_data.button_type;
            }
            break;
            }
        }

        if (mouse_x != -1 && mouse_button == WindowMessageMouseButtonType::left)
        {
            Mat4 inverse_projection;
            ASSERT(inverse(scene_uniform_data->projection, &inverse_projection));
            Mat4 inverse_view;
            ASSERT(inverse(scene_uniform_data->view, &inverse_view));

            Vec4 mouse_pos_clip = {(Real)mouse_x * 2 / window_width - 1, (Real)mouse_y * 2 / window_height - 1, 0, 1};
            Vec3 mouse_pos = vec3(perspective_divide(inverse_view * inverse_projection * mouse_pos_clip));
            Vec3 dir = normalize(mouse_pos - camera.pos);
            Ray ray;
            ray.pos = camera.pos;
            ray.dir = dir;

            Real min_dist = 10000000;
            Int clicked_piece_index = -1;
            for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
            {
                Piece *piece = &pieces[piece_i];
                if (piece->game_piece->side == game_state.player_side)
                {
                    CollisionBox collision_box;
                    collision_box.center = piece->collision_box.center + piece->pos;
                    collision_box.radius = piece->collision_box.radius;

                    Real dist = check_collision(&ray, &collision_box);
                    if (dist > 0 && dist < min_dist)
                    {
                        clicked_piece_index = piece_i;
                    }
                }
            }
            if (clicked_piece_index != -1)
            {
                game_state.selected_piece_index = clicked_piece_index;
            }

            if (game_state.selected_piece_index != -1 && clicked_piece_index == -1)
            {
                Int selected_row = -1;
                Int selected_column = -1;
                for (Int square_i = 0; square_i < BOARD_SQUARE_COUNT; square_i++)
                {
                    Real dist = check_collision(&ray, &board.collision_box[square_i]);
                    if (dist > 0)
                    {
                        selected_row = get_row(square_i);
                        selected_column = get_column(square_i);
                        break;
                    }
                }

                if (selected_row != -1 && selected_column != -1)
                {
                    GamePiece *game_piece = &game_state.pieces[game_state.selected_piece_index];
                    Bool can_move = check_game_move(&game_state, game_piece, selected_row, selected_column);
                    if (can_move)
                    {
                        Piece *piece = &pieces[game_state.selected_piece_index];
                        if (game_piece->type == GamePieceType::knight)
                        {
                            start_jump_animation(piece, selected_row, selected_column);
                        }
                        else
                        {
                            start_move_animation(piece, selected_row, selected_column);
                        }
                        update_piece_pos(&game_state, game_piece, selected_row, selected_column);
                        game_state.selected_piece_index = -1;
                    }
                }
            }
        }
        else if (mouse_x != -1 && mouse_button == WindowMessageMouseButtonType::right)
        {
            game_state.selected_piece_index = -1;
        }

        Vec3 camera_x = rotate(camera.rotation, get_basis_x());
        Vec3 camera_y = rotate(camera.rotation, get_basis_y());
        Vec3 camera_z = rotate(camera.rotation, get_basis_z());
        Real speed = 3;
        if (moving_x_pos)
        {
            camera.pos = camera.pos + speed * camera_x;
        }
        if (moving_x_neg)
        {
            camera.pos = camera.pos - speed * camera_x;
        }
        if (moving_y_pos)
        {
            camera.pos = camera.pos + speed * camera_y;
        }
        if (moving_y_neg)
        {
            camera.pos = camera.pos - speed * camera_y;
        }
        if (moving_z_pos)
        {
            camera.pos = camera.pos + speed * camera_z;
        }
        if (moving_z_neg)
        {
            camera.pos = camera.pos - speed * camera_z;
        }

        Real rotating_speed = degree_to_radian(0.2);
        Quaternion local_rotation = get_identity_quaternion();
        if (rotating_x_pos)
        {
            local_rotation = get_rotation_quaternion(get_basis_x(), rotating_speed) * local_rotation;
        }
        if (rotating_x_neg)
        {
            local_rotation = get_rotation_quaternion(get_basis_x(), -rotating_speed) * local_rotation;
        }
        if (rotating_y_pos)
        {
            local_rotation = get_rotation_quaternion(get_basis_y(), rotating_speed) * local_rotation;
        }
        if (rotating_y_neg)
        {
            local_rotation = get_rotation_quaternion(get_basis_y(), -rotating_speed) * local_rotation;
        }
        if (rotating_z_pos)
        {
            local_rotation = get_rotation_quaternion(get_basis_z(), rotating_speed) * local_rotation;
        }
        if (rotating_z_neg)
        {
            local_rotation = get_rotation_quaternion(get_basis_z(), -rotating_speed) * local_rotation;
        }
        camera.rotation = camera.rotation * local_rotation;

        calculate_scene_uniform_data(&camera, window_width, window_height, scene_uniform_data);
        for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
        {
            Piece *piece = &pieces[piece_i];
            update_animation(piece, 0.1);
            calculate_entity_uniform_data(piece, get_piece_uniform_data(&device, &scene_uniform_buffer, piece_i));
        }

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

            debug_ui_draw_str(&debug_ui_draw_state, str("rotation: "));
            debug_ui_draw_vec4(&debug_ui_draw_state, vec4(camera.rotation));
            debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_indent(&debug_ui_draw_state, -1);

            // debug_ui_draw_str(&debug_ui_draw_state, str("ray"));
            // debug_ui_draw_newline(&debug_ui_draw_state);
            // debug_ui_draw_indent(&debug_ui_draw_state, 1);

            // debug_ui_draw_str(&debug_ui_draw_state, str("position: "));
            // debug_ui_draw_vec3(&debug_ui_draw_state, ray.pos);
            // debug_ui_draw_newline(&debug_ui_draw_state);

            // debug_ui_draw_str(&debug_ui_draw_state, str("dir: "));
            // debug_ui_draw_vec3(&debug_ui_draw_state, ray.dir);
            // debug_ui_draw_newline(&debug_ui_draw_state);

            debug_ui_draw_str(&debug_ui_draw_state, str("selected piece: "));
            if (game_state.selected_piece_index != -1)
            {
                debug_ui_draw_str(&debug_ui_draw_state, pieces[game_state.selected_piece_index].game_piece->name);
            }
            else
            {
                debug_ui_draw_str(&debug_ui_draw_state, str("<no>"));
            }
            debug_ui_draw_newline(&debug_ui_draw_state);

            // debug_ui_draw_str(&debug_ui_draw_state, str("selected square: "));
            // if (selected_row != -1 && selected_column != -1)
            // {
            //     debug_ui_draw_int(&debug_ui_draw_state, selected_row + 1);
            //     debug_ui_draw_str(&debug_ui_draw_state, str(", "));
            //     debug_ui_draw_int(&debug_ui_draw_state, selected_column + 1);
            // }
            // else
            // {
            //     debug_ui_draw_str(&debug_ui_draw_state, str("<no>"));
            // }
            // debug_ui_draw_newline(&debug_ui_draw_state);
        }

        ASSERT(render_vulkan_frame(&device,
                                   &scene_pipeline, &scene_frame, &scene_uniform_buffer, &board, pieces,
                                   &debug_ui_pipeline, &debug_ui_frame, &debug_ui_vertex_buffer, debug_ui_draw_state.character_count,
                                   &debug_collision_pipeline, &debug_collision_frame, &debug_collision_vertex_buffer));
    }

    return 0;
}

#include "../lib/util.cpp"
#include "../lib/vulkan.cpp"
#include "../lib/os.cpp"
