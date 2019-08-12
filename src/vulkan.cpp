#pragma once

#include "util.cpp"
#include "window.cpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

struct VulkanContext
{
    VkInstance instance_handle;
    VkPhysicalDevice physical_device_handle;
    VkDevice device_handle;
    UInt4 graphics_queue_index;
    VkQueue graphics_queue;
    VkSurfaceKHR surface_handle;
    UInt4 present_queue_index;
    VkQueue present_queue;
};

Bool create_vulkan_context(VulkanContext *context, Window window)
{
    VkResult vulkan_result;

    VkApplicationInfo application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        NULL,
        "",
        VK_MAKE_VERSION(0, 0, 0),
        "",
        VK_MAKE_VERSION(0, 0, 0),
        VK_MAKE_VERSION(0, 0, 0),
    };

    RawStr instance_layers[1] = {"VK_LAYER_LUNARG_standard_validation"};
    RawStr instance_extensions[3] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME};
    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        NULL,
        0,
        &application_info,
        1,
        instance_layers,
        3,
        instance_extensions,
    };

    vulkan_result = vkCreateInstance(&instance_create_info, NULL, &context->instance_handle);
    if (vulkan_result != VK_SUCCESS)
    {
        return false;
    }

    UInt4 physical_devices_count = 0;
    vulkan_result = vkEnumeratePhysicalDevices(context->instance_handle, &physical_devices_count, NULL);
    if (vulkan_result != VK_SUCCESS || physical_devices_count < 1)
    {
        return false;
    }

    UInt4 desired_physical_devices_count = 1;
    vulkan_result = vkEnumeratePhysicalDevices(context->instance_handle, &desired_physical_devices_count, &context->physical_device_handle);
    if (vulkan_result != VK_SUCCESS)
    {
        return false;
    }

    VkWin32SurfaceCreateInfoKHR surface_create_info = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        NULL,
        0,
        GetModuleHandleA(NULL),
        (HWND)window,
    };

    vulkan_result = vkCreateWin32SurfaceKHR(context->instance_handle, &surface_create_info, NULL, &context->surface_handle);
    if (vulkan_result != VK_SUCCESS)
    {
        return false;
    }

    UInt4 queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device_handle, &queue_families_count, NULL);

    VkQueueFamilyProperties queue_families_properties[8];
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device_handle, &queue_families_count, queue_families_properties);

    context->graphics_queue_index = -1;
    for (Int queue_family_index = 0; queue_family_index < queue_families_count; queue_family_index++)
    {
        if (queue_families_properties[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            context->graphics_queue_index = queue_family_index;
            break;
        }
    }

    if (context->graphics_queue_index == -1)
    {
        return false;
    }

    context->present_queue_index = -1;
    for (Int queue_family_index = 0; queue_family_index < queue_families_count; queue_family_index++)
    {
        VkBool32 is_supported;
        vulkan_result = vkGetPhysicalDeviceSurfaceSupportKHR(context->physical_device_handle, queue_family_index, context->surface_handle, &is_supported);
        if (vulkan_result == VK_SUCCESS && is_supported)
        {
            context->present_queue_index = queue_family_index;
            break;
        }
    }

    if (context->present_queue_index == -1)
    {
        return false;
    }

    Real4 priority = 1.0f;
    Int queues_create_count = 1;
    VkDeviceQueueCreateInfo queues_create_info[2] = {
        {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            NULL,
            0,
            context->graphics_queue_index,
            1,
            &priority,
        },
    };
    if (context->present_queue_index != context->graphics_queue_index)
    {
        queues_create_info[1] = {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            NULL,
            0,
            context->present_queue_index,
            1,
            &priority,
        },
        queues_create_count++;
    }

    RawStr device_extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo device_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        NULL,
        0,
        (UInt4)queues_create_count,
        queues_create_info,
        0,
        NULL,
        1,
        device_extensions,
        NULL,
    };

    vulkan_result = vkCreateDevice(context->physical_device_handle, &device_create_info, NULL, &context->device_handle);
    if (vulkan_result != VK_SUCCESS)
    {
        return false;
    }

    vkGetDeviceQueue(context->device_handle, context->graphics_queue_index, 0, &context->graphics_queue);
    vkGetDeviceQueue(context->device_handle, context->present_queue_index, 0, &context->present_queue);

    return true;
}

struct VulkanSwapchain
{
    VkSwapchainKHR handle;
    Array<VkImage> images;
};

Bool create_vulkan_swapchain(VulkanContext *context, VulkanSwapchain *swapchain, UInt4 width, UInt4 height, VulkanSwapchain *old_swapchain = NULL)
{
    VkSwapchainCreateInfoKHR swapchain_create_info = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        NULL,
        0,
        context->surface_handle,
        3,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        {width, height},
        1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_PRESENT_MODE_FIFO_KHR,
        VK_TRUE,
        old_swapchain ? old_swapchain->handle : VK_NULL_HANDLE,
    };

    VkResult vulkan_result;
    vulkan_result = vkCreateSwapchainKHR(context->device_handle, &swapchain_create_info, NULL, &swapchain->handle);
    if (vulkan_result != VK_SUCCESS)
    {
        return false;
    }

    UInt4 swapchain_images_count = 0;
    vulkan_result = vkGetSwapchainImagesKHR(context->device_handle, swapchain->handle, &swapchain_images_count, NULL);
    if (vulkan_result != VK_SUCCESS)
    {
        return false;
    }

    swapchain->images = create_array<VkImage>(swapchain_images_count);
    swapchain->images.length = swapchain_images_count;
    vulkan_result = vkGetSwapchainImagesKHR(context->device_handle, swapchain->handle, &swapchain_images_count, swapchain->images.data);
    if (vulkan_result != VK_SUCCESS)
    {
        return false;
    }

    if (old_swapchain != NULL)
    {
        vkDestroySwapchainKHR(context->device_handle, old_swapchain->handle, NULL);
    }

    return true;
}
