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

        assert(render_vulkan_frame(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline, &vulkan_frames[current_frame_index]));
        current_frame_index = (current_frame_index + 1) % 3;
    }

    return 0;
}
