#include "util.hpp"
#include "os.hpp"
#include "vulkan.hpp"

Void debug_callback(Str message)
{
}

Int main()
{
    Handle window = create_window(str("test"), 800, 600, 0, 0);
    ASSERT(window);

    VulkanDevice device;
    ASSERT(create_vulkan_device(window, debug_callback, &device));
    ASSERT(create_vulkan_swapchain(&device, 800, 600, 3, VK_PRESENT_MODE_MAILBOX_KHR));
}

#include "util.cpp"
#include "os.cpp"
#include "vulkan.cpp"
