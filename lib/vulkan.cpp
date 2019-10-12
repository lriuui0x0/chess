#include "os.hpp"
#include "vulkan.hpp"

VkBool32 vulkan_debug_callback(VkDebugReportFlagsEXT flags,
                               VkDebugReportObjectTypeEXT object_type,
                               uint64_t object,
                               size_t location,
                               int32_t message_code,
                               const char *layer_prefix,
                               const char *message,
                               void *user_data)
{
    VulkanDebugCallback debug_callback = (VulkanDebugCallback)user_data;
    debug_callback(str(message));
    return VK_FALSE;
}

Bool create_device(Handle window, VulkanDebugCallback debug_callback, VulkanDevice *device)
{
    // TODO: Platform independent library name
    Handle library = load_library(str("vulkan-1.dll"));
    if (!library)
    {
        return false;
    }

#undef VK_FUNC
#define VK_FUNC(func_name)                                                        \
    {                                                                             \
        func_name = (PFN_##func_name)load_library_func(library, str(#func_name)); \
        if (!func_name)                                                           \
        {                                                                         \
            return false;                                                         \
        }                                                                         \
    }
    VK_FUNC_LIST_EXPORT

#undef VK_FUNC
#define VK_FUNC(func_name)                                                    \
    {                                                                         \
        func_name = (PFN_##func_name)vkGetInstanceProcAddr(null, #func_name); \
        if (!func_name)                                                       \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }
    VK_FUNC_LIST_GLOBAL

    VkResult result_code;
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "";
    application_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    application_info.pEngineName = "";
    application_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    application_info.apiVersion = VK_MAKE_VERSION(0, 0, 0);

    // TODO: Enumerate layers & extensions
    CStr instance_layers[1] = {"VK_LAYER_LUNARG_standard_validation"};
    // TODO: Platform independent extension names
    CStr instance_extensions[3] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME};
    VkInstanceCreateInfo instance_create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = instance_layers;
    instance_create_info.enabledExtensionCount = 3;
    instance_create_info.ppEnabledExtensionNames = instance_extensions;

    result_code = vkCreateInstance(&instance_create_info, null, &device->instance);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

#undef VK_FUNC
#define VK_FUNC(func_name)                                                                \
    {                                                                                     \
        func_name = (PFN_##func_name)vkGetInstanceProcAddr(device->instance, #func_name); \
        if (!func_name)                                                                   \
        {                                                                                 \
            return false;                                                                 \
        }                                                                                 \
    }
    VK_FUNC_LIST_INSTANCE

    VkDebugReportCallbackCreateInfoEXT callback_create_info;
    callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callback_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                 VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                 VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callback_create_info.pUserData = (Void *)debug_callback;
    callback_create_info.pfnCallback = vulkan_debug_callback;

    result_code = vkCreateDebugReportCallbackEXT(device->instance, &callback_create_info, null, &device->debug_callback);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    // TODO: Platform indepedent surface creation
    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.hinstance = GetModuleHandleA(null);
    surface_create_info.hwnd = (HWND)window;

    result_code = vkCreateWin32SurfaceKHR(device->instance, &surface_create_info, null, &device->surface);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    // NOTE: Setting swapchain to null so that we won't try to free the old swapchain when we first create it
    device->swapchain.handle = VK_NULL_HANDLE;

    UInt32 physical_device_count = 0;
    result_code = vkEnumeratePhysicalDevices(device->instance, &physical_device_count, null);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }
    if (physical_device_count == 0)
    {
        return false;
    }

    VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)ALLOCA(physical_device_count * sizeof(VkPhysicalDevice));
    result_code = vkEnumeratePhysicalDevices(device->instance, &physical_device_count, physical_devices);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    device->physical_device = VK_NULL_HANDLE;
    for (Int physical_device_i = 0; physical_device_i < (Int)physical_device_count; physical_device_i++)
    {
        VkPhysicalDevice physical_device = physical_devices[physical_device_i];

        vkGetPhysicalDeviceFeatures(physical_device, &device->physical_device_features);
        vkGetPhysicalDeviceProperties(physical_device, &device->physical_device_properties);
        vkGetPhysicalDeviceMemoryProperties(physical_device, &device->physical_device_memory_properties);

        UInt32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, null);

        VkQueueFamilyProperties *queue_family_properties_all = (VkQueueFamilyProperties *)ALLOCA(queue_family_count * sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties_all);

        // TODO: Use same queue family when possible, so that there's no need for inter-queue barrier. i.e. prefer 2,2 over 0,1
        Int graphics_queue_index = -1;
        Int present_queue_index = -1;
        for (Int queue_family_i = 0; queue_family_i < (Int)queue_family_count; queue_family_i++)
        {
            VkQueueFamilyProperties *queue_family_properties = &queue_family_properties_all[queue_family_i];
            if (HAS_FLAG(queue_family_properties->queueFlags, VK_QUEUE_GRAPHICS_BIT))
            {
                graphics_queue_index = queue_family_i;
            }

            VkBool32 present_supported = false;
            result_code = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_i, device->surface, &present_supported);
            if (result_code == VK_SUCCESS && present_supported)
            {
                present_queue_index = queue_family_i;
            }

            if (graphics_queue_index != -1 && present_queue_index != -1)
            {
                break;
            }
        }

        if (graphics_queue_index != -1 && present_queue_index != -1)
        {
            device->physical_device = physical_device;
            device->graphics_queue_index = graphics_queue_index;
            device->present_queue_index = present_queue_index;
            break;
        }
    }

    if (device->physical_device == VK_NULL_HANDLE)
    {
        return false;
    }

    Real priority = 1.0f;
    Int queue_create_info_count = 1;
    VkDeviceQueueCreateInfo queue_create_info[2] = {};
    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].queueFamilyIndex = device->graphics_queue_index;
    queue_create_info[0].queueCount = 1;
    queue_create_info[0].pQueuePriorities = &priority;

    if (device->present_queue_index != device->graphics_queue_index)
    {
        queue_create_info_count++;
        queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[1].queueFamilyIndex = device->present_queue_index;
        queue_create_info[1].queueCount = 1;
        queue_create_info[1].pQueuePriorities = &priority;
    }

    CStr device_extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = (UInt32)queue_create_info_count;
    device_create_info.pQueueCreateInfos = queue_create_info;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = null;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = device_extensions;
    device_create_info.pEnabledFeatures = null;

    result_code = vkCreateDevice(device->physical_device, &device_create_info, null, &device->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

#undef VK_FUNC
#define VK_FUNC(func_name)                                                            \
    {                                                                                 \
        func_name = (PFN_##func_name)vkGetDeviceProcAddr(device->handle, #func_name); \
        if (!func_name)                                                               \
        {                                                                             \
            return false;                                                             \
        }                                                                             \
    }
    VK_FUNC_LIST_DEVICE

    vkGetDeviceQueue(device->handle, device->graphics_queue_index, 0, &device->graphics_queue);
    vkGetDeviceQueue(device->handle, device->present_queue_index, 0, &device->present_queue);

    VkCommandPoolCreateInfo command_pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    command_pool_create_info.queueFamilyIndex = device->graphics_queue_index,

    result_code = vkCreateCommandPool(device->handle, &command_pool_create_info, null, &device->command_pool);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorPoolSize descriptor_pool_size[2];
    descriptor_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_size[0].descriptorCount = 48;

    descriptor_pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_size[1].descriptorCount = 16;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = 64;
    descriptor_pool_create_info.poolSizeCount = 2;
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_size;

    result_code = vkCreateDescriptorPool(device->handle, &descriptor_pool_create_info, null, &device->descriptor_pool);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool create_swapchain(VulkanDevice *device, Int width, Int height, Int desired_image_count, VkPresentModeKHR desired_present_mode)
{
    VkResult result_code;

    // NOTE: Set default present mode to FIFO
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    UInt32 present_mode_count;
    result_code = vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, device->surface, &present_mode_count, null);
    if (result_code == VK_SUCCESS)
    {
        VkPresentModeKHR *present_mode_all = (VkPresentModeKHR *)ALLOCA(present_mode_count * sizeof(VkPresentModeKHR));
        result_code = vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, device->surface, &present_mode_count, present_mode_all);
        if (result_code == VK_SUCCESS)
        {
            for (Int present_mode_i = 0; present_mode_i < (Int)present_mode_count; present_mode_i++)
            {
                VkPresentModeKHR found_present_mode = present_mode_all[present_mode_i];
                if (found_present_mode == desired_present_mode)
                {
                    present_mode = desired_present_mode;
                    break;
                }
            }
        }
    }

    VkFormat format;
    VkColorSpaceKHR color_space;
    UInt32 surface_format_count;
    result_code = vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device, device->surface, &surface_format_count, null);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkSurfaceFormatKHR *surface_format_all = (VkSurfaceFormatKHR *)ALLOCA(surface_format_count * sizeof(VkSurfaceFormatKHR));
    result_code = vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device, device->surface, &surface_format_count, surface_format_all);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    Bool found = false;
    for (Int surface_format_i = 0; surface_format_i < (Int)surface_format_count; surface_format_i++)
    {
        VkSurfaceFormatKHR *surface_format = &surface_format_all[surface_format_i];

        switch (surface_format->format)
        {
        case VK_FORMAT_UNDEFINED:
        {
            format = VK_FORMAT_R8G8B8A8_UNORM;
            found = true;
        }
        break;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        {
            format = surface_format->format;
            found = true;
        }
        break;

        default:
        {
            continue;
        }
        break;
        }

        if (found)
        {
            color_space = surface_format->colorSpace;
            break;
        }
    }

    if (!found)
    {
        return false;
    }
    device->swapchain.format = format;

    Int image_count = desired_image_count;
    VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    result_code = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, device->surface, &surface_capabilities);
    if (result_code == VK_SUCCESS)
    {
        transform = surface_capabilities.currentTransform;

        if (width < (Int)surface_capabilities.minImageExtent.width || width > (Int)surface_capabilities.maxImageExtent.width ||
            height < (Int)surface_capabilities.minImageExtent.height || height > (Int)surface_capabilities.maxImageExtent.height)
        {
            return false;
        }
        device->swapchain.width = width;
        device->swapchain.height = height;

        if (desired_image_count < (Int)surface_capabilities.minImageCount)
        {
            return false;
        }

        // NOTE: Clamped asked max image count
        if (surface_capabilities.maxImageCount > 0 && desired_image_count > (Int)surface_capabilities.maxImageCount)
        {
            image_count = surface_capabilities.maxImageCount;
        }
    }

    VkSwapchainKHR current_swapchain_handle = device->swapchain.handle;
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = device->surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = format;
    swapchain_create_info.imageColorSpace = color_space;
    swapchain_create_info.imageExtent.width = (UInt32)width;
    swapchain_create_info.imageExtent.height = (UInt32)height;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = null;
    swapchain_create_info.preTransform = transform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = current_swapchain_handle;

    result_code = vkCreateSwapchainKHR(device->handle, &swapchain_create_info, null, &device->swapchain.handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    if (current_swapchain_handle != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device->handle, current_swapchain_handle, null);
    }

    UInt32 swapchain_image_count = 0;
    result_code = vkGetSwapchainImagesKHR(device->handle, device->swapchain.handle, &swapchain_image_count, null);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    device->swapchain.images.count = MIN(desired_image_count, MIN((Int)swapchain_image_count, MAX_SWAPCHAIN_IMAGE_COUNT));
    result_code = vkGetSwapchainImagesKHR(device->handle, device->swapchain.handle, (UInt32 *)&device->swapchain.images.count, device->swapchain.images.data);
    if (result_code != VK_SUCCESS && result_code != VK_INCOMPLETE)
    {
        return false;
    }

    return true;
}

Bool create_render_pass(VulkanDevice *device, Buffer<AttachmentInfo> *color_attachments, AttachmentInfo *depth_attachment, AttachmentInfo *resolve_attachment, VkRenderPass *render_pass)
{
    Int color_attachments_count = color_attachments ? color_attachments->count : 0;
    Int attachment_count = color_attachments_count + (depth_attachment ? 1 : 0) + (resolve_attachment ? 1 : 0);
    VkAttachmentDescription *attachment_description_all = (VkAttachmentDescription *)ALLOCA(attachment_count * sizeof(VkAttachmentDescription));

    if (color_attachments_count)
    {
        for (Int color_attachment_i = 0; color_attachment_i < color_attachments_count; color_attachment_i++)
        {
            VkAttachmentDescription *attachment_description = &attachment_description_all[color_attachment_i];
            AttachmentInfo *attachment = &color_attachments->data[color_attachment_i];

            *attachment_description = {};
            attachment_description->format = attachment->format;
            attachment_description->samples = attachment->multisample_count;
            attachment_description->loadOp = attachment->load_op;
            attachment_description->storeOp = attachment->store_op;
            attachment_description->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_description->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment_description->initialLayout = attachment->initial_layout;
            attachment_description->finalLayout = attachment->final_layout;
        }
    }

    if (depth_attachment)
    {
        VkAttachmentDescription *attachment_description = &attachment_description_all[color_attachments_count];

        *attachment_description = {};
        attachment_description->format = depth_attachment->format;
        attachment_description->samples = depth_attachment->multisample_count;
        attachment_description->loadOp = depth_attachment->load_op;
        attachment_description->storeOp = depth_attachment->store_op;
        attachment_description->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_description->initialLayout = depth_attachment->initial_layout;
        attachment_description->finalLayout = depth_attachment->final_layout;
    }

    if (resolve_attachment)
    {
        VkAttachmentDescription *attachment_description = &attachment_description_all[color_attachments_count + 1];

        *attachment_description = {};
        attachment_description->format = resolve_attachment->format;
        attachment_description->samples = resolve_attachment->multisample_count;
        attachment_description->loadOp = resolve_attachment->load_op;
        attachment_description->storeOp = resolve_attachment->store_op;
        attachment_description->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_description->initialLayout = resolve_attachment->initial_layout;
        attachment_description->finalLayout = resolve_attachment->final_layout;
    }

    VkAttachmentReference *color_attachment_references;
    if (color_attachments_count)
    {
        color_attachment_references = (VkAttachmentReference *)ALLOCA(color_attachments_count * sizeof(VkAttachmentReference));
        for (Int color_attachment_i = 0; color_attachment_i < color_attachments_count; color_attachment_i++)
        {
            VkAttachmentReference *color_attachment_reference = &color_attachment_references[color_attachment_i];
            AttachmentInfo *attachment = &color_attachments->data[color_attachment_i];

            color_attachment_reference->attachment = color_attachment_i;
            color_attachment_reference->layout = attachment->working_layout;
        }
    }

    VkAttachmentReference depth_attachment_reference;
    if (depth_attachment)
    {
        depth_attachment_reference.attachment = color_attachments_count;
        depth_attachment_reference.layout = depth_attachment->working_layout;
    }

    VkAttachmentReference resolve_attachment_reference;
    if (resolve_attachment)
    {
        resolve_attachment_reference.attachment = color_attachments_count + 1;
        resolve_attachment_reference.layout = resolve_attachment->working_layout;
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = color_attachments_count;
    subpass.pColorAttachments = color_attachments_count ? color_attachment_references : null;
    subpass.pDepthStencilAttachment = depth_attachment ? &depth_attachment_reference : null;
    subpass.pResolveAttachments = resolve_attachment ? &resolve_attachment_reference : null;

    // TODO: Explicit subpass dependencies

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = attachment_count;
    render_pass_create_info.pAttachments = attachment_description_all;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    VkResult result_code = vkCreateRenderPass(device->handle, &render_pass_create_info, null, render_pass);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool create_pipeline(VulkanDevice *device,
                     VkRenderPass render_pass, Int subpass,
                     Buffer<ShaderInfo> *shaders,
                     Int vertex_stride, Buffer<VertexAttributeInfo> *vertex_attributes, VkPrimitiveTopology primitive_type,
                     Buffer<DescriptorSetInfo> *descriptor_sets, Buffer<PushConstantInfo> *push_constants,
                     VkSampleCountFlagBits multisample_count, Bool depth_enable, Bool alpha_blend_enable, DepthBias *depth_bias,
                     VulkanPipeline *pipeline)
{
    VkResult result_code;

    VkPipelineShaderStageCreateInfo *shader_stage_info = (VkPipelineShaderStageCreateInfo *)ALLOCA(shaders->count * sizeof(VkPipelineShaderStageCreateInfo));
    for (Int shader_i = 0; shader_i < shaders->count; shader_i++)
    {
        ShaderInfo *shader_info = &shaders->data[shader_i];

        VkShaderModuleCreateInfo shader_module_create_info = {};
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.codeSize = (size_t)shader_info->code.count;
        shader_module_create_info.pCode = (UInt32 *)shader_info->code.data;

        VkShaderModule shader_module;
        VkResult result_code = vkCreateShaderModule(device->handle, &shader_module_create_info, null, &shader_module);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }

        shader_stage_info[shader_i] = {};
        shader_stage_info[shader_i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_info[shader_i].stage = shader_info->stage;
        shader_stage_info[shader_i].module = shader_module;
        shader_stage_info[shader_i].pName = "main";
    }

    VkVertexInputBindingDescription vertex_binding_description = {};
    vertex_binding_description.binding = 0;
    vertex_binding_description.stride = vertex_stride;
    vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // TODO: Validate vertex info values
    // TODO: Allow more types of vertex info values
    VkVertexInputAttributeDescription *vertex_attribute_description = (VkVertexInputAttributeDescription *)ALLOCA(vertex_attributes->count * sizeof(VkVertexInputAttributeDescription));
    for (Int vertex_attribute_i = 0; vertex_attribute_i < vertex_attributes->count; vertex_attribute_i++)
    {
        VertexAttributeInfo *vertex_attribute = &vertex_attributes->data[vertex_attribute_i];
        VkFormat format;
        switch (vertex_attribute->count)
        {
        case 4 * 1:
        {
            format = VK_FORMAT_R32_SFLOAT;
        }
        break;

        case 4 * 2:
        {
            format = VK_FORMAT_R32G32_SFLOAT;
        }
        break;

        case 4 * 3:
        {
            format = VK_FORMAT_R32G32B32_SFLOAT;
        }
        break;

        default:
        {
            return false;
        }
        break;
        }

        vertex_attribute_description[vertex_attribute_i].binding = 0;
        vertex_attribute_description[vertex_attribute_i].location = vertex_attribute_i;
        vertex_attribute_description[vertex_attribute_i].format = format;
        vertex_attribute_description[vertex_attribute_i].offset = vertex_attribute->offset;
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
    vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_info.pVertexBindingDescriptions = &vertex_binding_description;
    vertex_input_state_info.vertexAttributeDescriptionCount = vertex_attributes->count;
    vertex_input_state_info.pVertexAttributeDescriptions = vertex_attribute_description;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = {};
    input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_info.topology = primitive_type;
    input_assembly_state_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (Real)device->swapchain.width;
    viewport.height = (Real)device->swapchain.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {(UInt32)device->swapchain.width, (UInt32)device->swapchain.height};

    VkPipelineViewportStateCreateInfo viewport_state_info = {};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state_info = {};
    rasterization_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_info.depthClampEnable = VK_FALSE;
    rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_info.cullMode = VK_CULL_MODE_NONE;
    rasterization_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_info.depthBiasEnable = depth_bias ? VK_TRUE : VK_FALSE;
    rasterization_state_info.depthBiasConstantFactor = depth_bias ? depth_bias->const_bias : 0;
    rasterization_state_info.depthBiasClamp = 0;
    rasterization_state_info.depthBiasSlopeFactor = depth_bias ? depth_bias->slope_bias : 0;
    rasterization_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_info = {};
    multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_info.rasterizationSamples = multisample_count;
    multisample_state_info.sampleShadingEnable = VK_FALSE;
    multisample_state_info.minSampleShading = 1.0f;
    multisample_state_info.pSampleMask = null;
    multisample_state_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = {};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.depthTestEnable = depth_enable ? VK_TRUE : VK_FALSE;
    depth_stencil_state_info.depthWriteEnable = depth_enable ? VK_TRUE : VK_FALSE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_info.front = {};
    depth_stencil_state_info.back = {};
    depth_stencil_state_info.minDepthBounds = 0.0f;
    depth_stencil_state_info.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState framebuffer_color_blend_state;
    framebuffer_color_blend_state.blendEnable = alpha_blend_enable ? VK_TRUE : VK_FALSE;
    framebuffer_color_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    framebuffer_color_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    framebuffer_color_blend_state.colorBlendOp = VK_BLEND_OP_ADD;
    framebuffer_color_blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    framebuffer_color_blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    framebuffer_color_blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
    framebuffer_color_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info = {};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &framebuffer_color_blend_state;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    // TODO: Validate the number of descriptor sets passed in is less then the maximum supported
    pipeline->descriptor_set_layouts.count = descriptor_sets->count;
    for (Int descriptor_set_i = 0; descriptor_set_i < descriptor_sets->count; descriptor_set_i++)
    {
        Buffer<DescriptorBindingInfo> *descriptor_bindings = &descriptor_sets->data[descriptor_set_i].bindings;

        VkDescriptorSetLayoutBinding *descriptor_set_layout_bindings = (VkDescriptorSetLayoutBinding *)ALLOCA(descriptor_bindings->count * sizeof(VkDescriptorSetLayoutBinding));
        for (Int descriptor_binding_i = 0; descriptor_binding_i < descriptor_bindings->count; descriptor_binding_i++)
        {
            DescriptorBindingInfo *descriptor_binding = &descriptor_bindings->data[descriptor_binding_i];

            descriptor_set_layout_bindings[descriptor_binding_i] = {};
            descriptor_set_layout_bindings[descriptor_binding_i].binding = descriptor_binding_i;
            descriptor_set_layout_bindings[descriptor_binding_i].descriptorType = descriptor_binding->type;
            descriptor_set_layout_bindings[descriptor_binding_i].descriptorCount = 1;
            descriptor_set_layout_bindings[descriptor_binding_i].stageFlags = descriptor_binding->stage;
        }

        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
        descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_info.bindingCount = descriptor_bindings->count;
        descriptor_set_layout_info.pBindings = descriptor_set_layout_bindings;

        result_code = vkCreateDescriptorSetLayout(device->handle, &descriptor_set_layout_info, null, &pipeline->descriptor_set_layouts[descriptor_set_i]);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }
    }

    VkPushConstantRange *push_constant_ranges = null;
    if (push_constants)
    {
        push_constant_ranges = (VkPushConstantRange *)ALLOCA(sizeof(VkPushConstantRange) * push_constants->count);

        Int offset = 0;
        for (Int i = 0; i < push_constants->count; i++)
        {
            PushConstantInfo *push_constant = &push_constants->data[i];
            VkPushConstantRange *push_constant_range = &push_constant_ranges[i];
            push_constant_range->stageFlags = push_constant->stage;
            push_constant_range->offset = offset;
            push_constant_range->size = push_constant->size;
            offset += push_constant->size;

            if (offset % 4 != 0)
            {
                return false;
            }
        }
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = descriptor_sets->count;
    pipeline_layout_create_info.pSetLayouts = pipeline->descriptor_set_layouts.data;
    pipeline_layout_create_info.pushConstantRangeCount = push_constants ? push_constants->count : 0;
    pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges;

    result_code = vkCreatePipelineLayout(device->handle, &pipeline_layout_create_info, null, &pipeline->layout);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    pipeline->render_pass = render_pass;

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = shaders->count;
    pipeline_create_info.pStages = shader_stage_info;
    pipeline_create_info.pVertexInputState = &vertex_input_state_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_info;
    pipeline_create_info.pTessellationState = null;
    pipeline_create_info.pViewportState = &viewport_state_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_info;
    pipeline_create_info.pMultisampleState = &multisample_state_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_info;
    pipeline_create_info.pDynamicState = null;
    pipeline_create_info.layout = pipeline->layout;
    pipeline_create_info.renderPass = render_pass;
    pipeline_create_info.subpass = subpass;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    result_code = vkCreateGraphicsPipelines(device->handle, VK_NULL_HANDLE, 1, &pipeline_create_info, null, &pipeline->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool create_semaphore(VulkanDevice *device, VkSemaphore *semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result_code = vkCreateSemaphore(device->handle, &semaphore_create_info, null, semaphore);
    return result_code == VK_SUCCESS;
}

Bool create_fence(VulkanDevice *device, Bool signaled, VkFence *fence)
{
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u;

    VkResult result_code = vkCreateFence(device->handle, &fence_create_info, null, fence);
    return result_code == VK_SUCCESS;
}

Bool create_buffer(VulkanDevice *device, Int count, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_property, VulkanBuffer *buffer)
{
    VkResult result_code;

    buffer->count = count;
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = buffer->count;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result_code = vkCreateBuffer(device->handle, &buffer_create_info, null, &buffer->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device->handle, buffer->handle, &memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(device->physical_device, &memory_properties);

    VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
    for (Int memory_type_index = 0; memory_type_index < (Int)memory_properties.memoryTypeCount; memory_type_index++)
    {
        if (HAS_FLAG(memory_requirements.memoryTypeBits, 1 << memory_type_index) &&
            HAS_FLAG(memory_properties.memoryTypes[memory_type_index].propertyFlags, memory_property))
        {
            VkMemoryAllocateInfo buffer_memory_allocate_info = {};
            buffer_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            buffer_memory_allocate_info.allocationSize = memory_requirements.size;
            buffer_memory_allocate_info.memoryTypeIndex = memory_type_index;

            result_code = vkAllocateMemory(device->handle, &buffer_memory_allocate_info, null, &buffer_memory);
            if (result_code == VK_SUCCESS)
            {
                break;
            }
            else
            {
                return false;
            }
        }
    }

    if (buffer_memory == VK_NULL_HANDLE)
    {
        return false;
    }

    result_code = vkBindBufferMemory(device->handle, buffer->handle, buffer_memory, 0);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    if (HAS_FLAG(memory_property, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
    {
        result_code = vkMapMemory(device->handle, buffer_memory, 0, buffer_create_info.size, 0, (Void **)&buffer->data);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }
    }
    else
    {
        buffer->data = null;
    }

    return true;
}

Bool create_image(VulkanDevice *device, Int width, Int height, VkFormat format, VkSampleCountFlagBits multisample_count, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_property, VkImage *image)
{
    VkResult result_code;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.extent.width = (UInt32)width;
    image_create_info.extent.height = (UInt32)height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = multisample_count;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = null;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    result_code = vkCreateImage(device->handle, &image_create_info, null, image);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device->handle, *image, &memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(device->physical_device, &memory_properties);

    VkDeviceMemory image_memory = VK_NULL_HANDLE;
    for (Int memory_type_index = 0; memory_type_index < (Int)memory_properties.memoryTypeCount; memory_type_index++)
    {
        if (HAS_FLAG(memory_requirements.memoryTypeBits, 1 << memory_type_index) &&
            HAS_FLAG(memory_properties.memoryTypes[memory_type_index].propertyFlags, memory_property))
        {
            VkMemoryAllocateInfo image_memory_allocate_info = {};
            image_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            image_memory_allocate_info.allocationSize = memory_requirements.size;
            image_memory_allocate_info.memoryTypeIndex = memory_type_index;

            result_code = vkAllocateMemory(device->handle, &image_memory_allocate_info, null, &image_memory);
            if (result_code == VK_SUCCESS)
            {
                break;
            }
            else
            {
                return false;
            }
        }
    }

    if (image_memory == VK_NULL_HANDLE)
    {
        return false;
    }

    result_code = vkBindImageMemory(device->handle, *image, image_memory, 0);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool create_image_view(VulkanDevice *device, VkImage image, VkFormat format, VkImageAspectFlags aspect_mask, VkImageView *image_view)
{
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = aspect_mask;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    VkResult result_code = vkCreateImageView(device->handle, &image_view_create_info, null, image_view);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool create_sampler(VulkanDevice *device, VkSampler *sampler)
{
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.maxAnisotropy = 16;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    VkResult result_code = vkCreateSampler(device->handle, &sampler_create_info, null, sampler);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }
    return true;
}

Bool allocate_command_buffer(VulkanDevice *device, VkCommandBuffer *command_buffer)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = device->command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    VkResult result_code = vkAllocateCommandBuffers(device->handle, &command_buffer_allocate_info, command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Void free_command_buffer(VulkanDevice *device, VkCommandBuffer command_buffer)
{
    vkFreeCommandBuffers(device->handle, device->command_pool, 1, &command_buffer);
}

Bool upload_buffer(VulkanDevice *device, VulkanBuffer *host_buffer, VulkanBuffer *device_buffer)
{
    VkResult result_code;

    VkCommandBuffer temp_command_buffer;
    if (!allocate_command_buffer(device, &temp_command_buffer))
    {
        return false;
    }

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result_code = vkBeginCommandBuffer(temp_command_buffer, &command_buffer_begin_info);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkBufferCopy buffer_copy = {};
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = host_buffer->count;
    vkCmdCopyBuffer(temp_command_buffer, host_buffer->handle, device_buffer->handle, 1, &buffer_copy);

    result_code = vkEndCommandBuffer(temp_command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = null;
    submit_info.pWaitDstStageMask = null;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &temp_command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = null;

    result_code = vkQueueSubmit(device->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    result_code = vkDeviceWaitIdle(device->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    free_command_buffer(device, temp_command_buffer);

    return true;
}

Bool upload_texture(VulkanDevice *device, VulkanBuffer *host_buffer, VkImage image, Int width, Int height)
{
    VkResult result_code;

    VkCommandBuffer temp_command_buffer;
    if (!allocate_command_buffer(device, &temp_command_buffer))
    {
        return false;
    }

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result_code = vkBeginCommandBuffer(temp_command_buffer, &command_buffer_begin_info);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkImageMemoryBarrier image_barrier = {};
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.image = image;
    image_barrier.srcAccessMask = 0;
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(temp_command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 1, &image_barrier);

    VkBufferImageCopy image_copy;
    image_copy.bufferOffset = 0;
    image_copy.bufferRowLength = 0;
    image_copy.bufferImageHeight = 0;
    image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.imageSubresource.mipLevel = 0;
    image_copy.imageSubresource.baseArrayLayer = 0;
    image_copy.imageSubresource.layerCount = 1;
    image_copy.imageOffset.x = 0;
    image_copy.imageOffset.y = 0;
    image_copy.imageOffset.z = 0;
    image_copy.imageExtent.width = width;
    image_copy.imageExtent.height = height;
    image_copy.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(temp_command_buffer, host_buffer->handle, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

    image_barrier.image = image;
    image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(temp_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, null, 0, null, 1, &image_barrier);

    result_code = vkEndCommandBuffer(temp_command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = null;
    submit_info.pWaitDstStageMask = null;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &temp_command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = null;

    result_code = vkQueueSubmit(device->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    result_code = vkDeviceWaitIdle(device->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    free_command_buffer(device, temp_command_buffer);

    return true;
}

Bool allocate_descriptor_set(VulkanDevice *device, VkDescriptorSetLayout descriptor_set_layout,
                             VulkanBuffer *uniform_buffer, Int offset, Int range, VkDescriptorSet *descriptor_set)
{
    VkResult result_code;

    VkDescriptorSetAllocateInfo entity_descriptor_set_alloc_info = {};
    entity_descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    entity_descriptor_set_alloc_info.descriptorPool = device->descriptor_pool;
    entity_descriptor_set_alloc_info.descriptorSetCount = 1;
    entity_descriptor_set_alloc_info.pSetLayouts = &descriptor_set_layout;

    result_code = vkAllocateDescriptorSets(device->handle, &entity_descriptor_set_alloc_info, descriptor_set);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = uniform_buffer->handle;
    descriptor_buffer_info.offset = offset;
    descriptor_buffer_info.range = range;

    VkWriteDescriptorSet descriptor_set_write = {};
    descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_set_write.dstSet = *descriptor_set;
    descriptor_set_write.dstBinding = 0;
    descriptor_set_write.dstArrayElement = 0;
    descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_write.descriptorCount = 1;
    descriptor_set_write.pBufferInfo = &descriptor_buffer_info;

    vkUpdateDescriptorSets(device->handle, 1, &descriptor_set_write, 0, null);

    return true;
}

Bool allocate_descriptor_set(VulkanDevice *device, VkDescriptorSetLayout descriptor_set_layout,
                             VkImageView image_view, VkSampler sampler, VkDescriptorSet *descriptor_set)
{
    VkResult result_code;

    VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {};
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool = device->descriptor_pool;
    descriptor_set_alloc_info.descriptorSetCount = 1;
    descriptor_set_alloc_info.pSetLayouts = &descriptor_set_layout;

    result_code = vkAllocateDescriptorSets(device->handle, &descriptor_set_alloc_info, descriptor_set);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorImageInfo descriptor_image_info;
    descriptor_image_info.sampler = sampler;
    descriptor_image_info.imageView = image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptor_set_write = {};
    descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_set_write.dstSet = *descriptor_set;
    descriptor_set_write.dstBinding = 0;
    descriptor_set_write.dstArrayElement = 0;
    descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_write.descriptorCount = 1;
    descriptor_set_write.pImageInfo = &descriptor_image_info;

    vkUpdateDescriptorSets(device->handle, 1, &descriptor_set_write, 0, null);

    return true;
}

VkSampleCountFlagBits get_maximum_multisample_count(VulkanDevice *device)
{
    VkSampleCountFlagBits multisample_count = VK_SAMPLE_COUNT_64_BIT;
    while (
        !(multisample_count & device->physical_device_properties.limits.framebufferColorSampleCounts) ||
        !(multisample_count & device->physical_device_properties.limits.framebufferDepthSampleCounts) ||
        !(multisample_count & device->physical_device_properties.limits.sampledImageColorSampleCounts) ||
        !(multisample_count & device->physical_device_properties.limits.sampledImageDepthSampleCounts))
    {
        multisample_count = (VkSampleCountFlagBits)((UInt)multisample_count >> 1);
    }
    return multisample_count;
}
