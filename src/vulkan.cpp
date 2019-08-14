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
    VkResult result_code;

    VkApplicationInfo application_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    application_info.pApplicationName = "";
    application_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    application_info.pEngineName = "";
    application_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    application_info.apiVersion = VK_MAKE_VERSION(0, 0, 0);

    RawStr instance_layers[1] = {"VK_LAYER_LUNARG_standard_validation"};
    RawStr instance_extensions[3] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME};
    VkInstanceCreateInfo instance_create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = instance_layers;
    instance_create_info.enabledExtensionCount = 3;
    instance_create_info.ppEnabledExtensionNames = instance_extensions;

    result_code = vkCreateInstance(&instance_create_info, NULL, &context->instance_handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    UInt4 physical_devices_count = 0;
    result_code = vkEnumeratePhysicalDevices(context->instance_handle, &physical_devices_count, NULL);
    if (result_code != VK_SUCCESS || physical_devices_count < 1)
    {
        return false;
    }

    UInt4 desired_physical_devices_count = 1;
    result_code = vkEnumeratePhysicalDevices(context->instance_handle, &desired_physical_devices_count, &context->physical_device_handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkWin32SurfaceCreateInfoKHR surface_create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    surface_create_info.hinstance = GetModuleHandleA(NULL);
    surface_create_info.hwnd = (HWND)window;

    result_code = vkCreateWin32SurfaceKHR(context->instance_handle, &surface_create_info, NULL, &context->surface_handle);
    if (result_code != VK_SUCCESS)
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
        result_code = vkGetPhysicalDeviceSurfaceSupportKHR(context->physical_device_handle, queue_family_index, context->surface_handle, &is_supported);
        if (result_code == VK_SUCCESS && is_supported)
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
    VkDeviceQueueCreateInfo queues_create_info[2];
    queues_create_info[0] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queues_create_info[0].queueFamilyIndex = context->graphics_queue_index;
    queues_create_info[0].queueCount = 1;
    queues_create_info[0].pQueuePriorities = &priority;

    if (context->present_queue_index != context->graphics_queue_index)
    {
        queues_create_info[1] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queues_create_info[1].queueFamilyIndex = context->present_queue_index;
        queues_create_info[1].queueCount = 1;
        queues_create_info[1].pQueuePriorities = &priority;
    }

    RawStr device_extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = (UInt4)queues_create_count;
    device_create_info.pQueueCreateInfos = queues_create_info;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = NULL;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = device_extensions;
    device_create_info.pEnabledFeatures = NULL;

    result_code = vkCreateDevice(context->physical_device_handle, &device_create_info, NULL, &context->device_handle);
    if (result_code != VK_SUCCESS)
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
    Int width;
    Int height;
    Array<VkImage> images;
    Array<VkImageView> image_views;
};

Bool create_vulkan_swapchain(VulkanContext *context, VulkanSwapchain *swapchain, Int width, Int height, VulkanSwapchain *old_swapchain = NULL)
{
    VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_create_info.surface = context->surface_handle;
    swapchain_create_info.minImageCount = 3;
    swapchain_create_info.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    swapchain_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_create_info.imageExtent.width = (UInt4)width;
    swapchain_create_info.imageExtent.height = (UInt4)height;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = NULL;
    swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = old_swapchain ? old_swapchain->handle : VK_NULL_HANDLE;

    VkResult result_code;
    result_code = vkCreateSwapchainKHR(context->device_handle, &swapchain_create_info, NULL, &swapchain->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    if (old_swapchain != NULL)
    {
        vkDestroySwapchainKHR(context->device_handle, old_swapchain->handle, NULL);
    }

    UInt4 swapchain_images_count = 0;
    result_code = vkGetSwapchainImagesKHR(context->device_handle, swapchain->handle, &swapchain_images_count, NULL);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    swapchain->images = create_array<VkImage>(swapchain_images_count);
    swapchain->images.length = swapchain_images_count;
    result_code = vkGetSwapchainImagesKHR(context->device_handle, swapchain->handle, &swapchain_images_count, swapchain->images.data);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    swapchain->image_views = create_array<VkImageView>(swapchain_images_count);
    swapchain->image_views.length = swapchain_images_count;
    for (Int i = 0; i < swapchain_images_count; i++)
    {
        VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        image_view_create_info.image = swapchain->images[i];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        result_code = vkCreateImageView(context->device_handle, &image_view_create_info, nullptr, &swapchain->image_views[i]);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

Bool create_vulkan_shader(VulkanContext *context, Str filename, VkShaderModule *shader_module)
{
    Str code;
    if (!read_file(filename, &code))
    {
        return false;
    }

    VkShaderModuleCreateInfo shader_module_create_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shader_module_create_info.codeSize = (size_t)code.length;
    shader_module_create_info.pCode = (UInt4 *)code.data;

    VkResult result_code = vkCreateShaderModule(context->device_handle, &shader_module_create_info, NULL, shader_module);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool create_vulkan_pipeline(VulkanContext *context, VulkanSwapchain *swapchain)
{
    VkResult result_code;

    VkAttachmentDescription framebuffer_description = {};
    framebuffer_description.format = VK_FORMAT_R8G8B8A8_UNORM;
    framebuffer_description.samples = VK_SAMPLE_COUNT_1_BIT;
    framebuffer_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    framebuffer_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    framebuffer_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    framebuffer_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    framebuffer_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    framebuffer_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference framebuffer_reference = {};
    framebuffer_reference.attachment = 0;
    framebuffer_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &framebuffer_reference;

    VkRenderPassCreateInfo render_pass_create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &framebuffer_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    VkRenderPass render_pass;
    result_code = vkCreateRenderPass(context->device_handle, &render_pass_create_info, NULL, &render_pass);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkShaderModule vert_shader_module;
    if (!create_vulkan_shader(context, wrap_str("vert.spv"), &vert_shader_module))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = vert_shader_module;
    vertex_shader_stage_create_info.pName = "main";

    VkShaderModule frag_shader_module;
    if (!create_vulkan_shader(context, wrap_str("frag.spv"), &frag_shader_module))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_create_info.module = frag_shader_module;
    fragment_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stage_create_info[2] = {vertex_shader_stage_create_info, fragment_shader_stage_create_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (Real4)swapchain->width;
    viewport.height = (Real4)swapchain->height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {(UInt4)swapchain->width, (UInt4)swapchain->height};

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_create_info.depthBiasClamp = 0.0f;
    rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
    rasterization_state_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multi_sample_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multi_sample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multi_sample_state_create_info.sampleShadingEnable = VK_FALSE;
    multi_sample_state_create_info.minSampleShading = 1.0f;
    multi_sample_state_create_info.pSampleMask = NULL;
    multi_sample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multi_sample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState framebuffer_color_blend_state;
    framebuffer_color_blend_state.blendEnable = VK_FALSE;
    framebuffer_color_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    framebuffer_color_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    framebuffer_color_blend_state.colorBlendOp = VK_BLEND_OP_ADD;
    framebuffer_color_blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    framebuffer_color_blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    framebuffer_color_blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
    framebuffer_color_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &framebuffer_color_blend_state;
    color_blend_state_create_info.blendConstants[0] = 0.0f;
    color_blend_state_create_info.blendConstants[1] = 0.0f;
    color_blend_state_create_info.blendConstants[2] = 0.0f;
    color_blend_state_create_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_create_info.setLayoutCount = 0;
    pipeline_layout_create_info.pSetLayouts = NULL;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = NULL;

    VkPipelineLayout pipeline_layout;
    result_code = vkCreatePipelineLayout(context->device_handle, &pipeline_layout_create_info, NULL, &pipeline_layout);

    VkGraphicsPipelineCreateInfo pipeline_create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stage_create_info;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pTessellationState = NULL;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multi_sample_state_create_info;
    pipeline_create_info.pDepthStencilState = NULL;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = NULL;
    pipeline_create_info.layout = pipeline_layout;
    pipeline_create_info.renderPass = render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    VkPipeline pipeline;
    result_code = vkCreateGraphicsPipelines(context->device_handle, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}
