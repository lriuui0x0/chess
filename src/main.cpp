#include "util.cpp"
#include "window.cpp"
#include "vulkan.cpp"

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    Window window = create_window(wrap_str("Chess"), 800, 600, 50, 50);
    assert(window);

    VulkanContext vulkan_context;
    assert(create_vulkan_context(window, &vulkan_context));

    VulkanSwapchain vulkan_swapchain;
    assert(create_vulkan_swapchain(&vulkan_context, 800, 600, &vulkan_swapchain));

    VulkanPipeline vulkan_pipeline;
    assert(create_vulkan_pipeline(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline));

    Array<VulkanFrame> vulkan_frames;
    assert(create_vulkan_frame(&vulkan_context, 3, &vulkan_frames));

    Vertex vertices[4] = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
    };
    UInt2 vertex_indices[6] = {0, 1, 2, 2, 3, 0};

    VulkanBuffer staging_vertex_buffer;
    create_vulkan_buffer(&vulkan_context, sizeof(vertices),
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &staging_vertex_buffer);
    memcpy(staging_vertex_buffer.data, vertices, sizeof(vertices));

    VulkanBuffer vertex_buffer;
    create_vulkan_buffer(&vulkan_context, sizeof(vertices),
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         &vertex_buffer);

    VulkanBuffer staging_index_buffer;
    create_vulkan_buffer(&vulkan_context, sizeof(vertex_indices),
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &staging_index_buffer);
    memcpy(staging_index_buffer.data, vertex_indices, sizeof(vertex_indices));

    VulkanBuffer index_buffer;
    create_vulkan_buffer(&vulkan_context, sizeof(vertex_indices),
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         &index_buffer);

    VkCommandBufferAllocateInfo command_buffer_alloc_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandPool = vulkan_context.command_pool;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(vulkan_context.device_handle, &command_buffer_alloc_info, &command_buffer);

    VkCommandBufferBeginInfo command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    VkBufferCopy vertex_buffer_copy = {};
    vertex_buffer_copy.srcOffset = 0;
    vertex_buffer_copy.dstOffset = 0;
    vertex_buffer_copy.size = vertex_buffer.length;
    vkCmdCopyBuffer(command_buffer, staging_vertex_buffer.handle, vertex_buffer.handle, 1, &vertex_buffer_copy);

    VkBufferCopy index_buffer_copy = {};
    index_buffer_copy.srcOffset = 0;
    index_buffer_copy.dstOffset = 0;
    index_buffer_copy.size = index_buffer.length;
    vkCmdCopyBuffer(command_buffer, staging_index_buffer.handle, index_buffer.handle, 1, &index_buffer_copy);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(vulkan_context.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkan_context.graphics_queue);

    show_window(window);

    bool is_running = true;
    Int current_frame_index = 0;
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
        }

        assert(render_vulkan_frame(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline, &vulkan_frames[current_frame_index], vertex_buffer.handle, index_buffer.handle));
        current_frame_index = (current_frame_index + 1) % 3;
    }

    return 0;
}
