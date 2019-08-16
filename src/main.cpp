#include "util.cpp"
#include "math.cpp"
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

    Vertex vertices[4] = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
    };
    UInt2 vertex_indices[6] = {0, 1, 2, 2, 3, 0};

    VulkanPipeline vulkan_pipeline;
    assert(create_vulkan_pipeline(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline));

    Array<VulkanFrame> vulkan_frames;
    assert(create_vulkan_frame(&vulkan_context, 3, &vulkan_frames));

    VulkanBuffer host_vertex_buffer;
    assert(create_vulkan_buffer(&vulkan_context, sizeof(vertices),
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &host_vertex_buffer));
    memcpy(host_vertex_buffer.data, vertices, sizeof(vertices));

    VulkanBuffer vertex_buffer;
    assert(create_vulkan_buffer(&vulkan_context, sizeof(vertices),
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &vertex_buffer));

    VulkanBuffer host_index_buffer;
    assert(create_vulkan_buffer(&vulkan_context, sizeof(vertex_indices),
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &host_index_buffer));
    memcpy(host_index_buffer.data, vertex_indices, sizeof(vertex_indices));

    VulkanBuffer index_buffer;
    assert(create_vulkan_buffer(&vulkan_context, sizeof(vertex_indices),
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &index_buffer));

    Transform transform = {
        {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
    };

    VulkanBuffer host_transform_buffer;
    assert(create_vulkan_buffer(&vulkan_context, sizeof(transform),
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &host_transform_buffer));
    memcpy(host_transform_buffer.data, &transform, sizeof(transform));

    VulkanBuffer transform_buffer;
    assert(create_vulkan_buffer(&vulkan_context, sizeof(transform),
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &transform_buffer));

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

        assert(render_vulkan_frame(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline, &vulkan_frames[current_frame_index],
                                   &host_vertex_buffer, &vertex_buffer, &host_index_buffer, &index_buffer, &host_transform_buffer, &transform_buffer));
        current_frame_index = (current_frame_index + 1) % 3;
    }

    return 0;
}
