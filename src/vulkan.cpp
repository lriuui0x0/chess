#pragma once

#include "util.cpp"
#include "math.cpp"
#include "window.cpp"
#include "model.cpp"

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
    VkCommandPool command_pool;
    VkDescriptorPool descriptor_pool;
};

PFN_vkCreateDebugReportCallbackEXT create_debug_callback = VK_NULL_HANDLE;

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags,
                                              VkDebugReportObjectTypeEXT objectType,
                                              uint64_t object,
                                              size_t location,
                                              int32_t messageCode,
                                              const char *pLayerPrefix,
                                              const char *pMessage,
                                              void *pUserData)
{
    OutputDebugStringA(pMessage);
    OutputDebugStringA("\n");
    return VK_FALSE;
}

Bool create_vulkan_context(Window window, OUT VulkanContext *context)
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

    create_debug_callback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(context->instance_handle, "vkCreateDebugReportCallbackEXT");

    VkDebugReportCallbackCreateInfoEXT callback_create_info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};
    callback_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                 VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                 VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callback_create_info.pfnCallback = &debug_callback;

    VkDebugReportCallbackEXT callback;
    result_code = create_debug_callback(context->instance_handle, &callback_create_info, NULL, &callback);
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

    Real priority = 1.0f;
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

    VkCommandPoolCreateInfo command_pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    command_pool_create_info.queueFamilyIndex = context->graphics_queue_index,

    result_code = vkCreateCommandPool(context->device_handle, &command_pool_create_info, NULL, &context->command_pool);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorPoolSize descriptor_pool_size = {};
    descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;

    result_code = vkCreateDescriptorPool(context->device_handle, &descriptor_pool_create_info, NULL, &context->descriptor_pool);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

struct VulkanSwapchain
{
    VkSwapchainKHR handle;
    Int width;
    Int height;
    Array<VkImage> images;
};

Bool create_vulkan_swapchain(VulkanContext *context, Int width, Int height, OUT VulkanSwapchain *swapchain)
{
    swapchain->width = width;
    swapchain->height = height;

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
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    VkResult result_code;
    result_code = vkCreateSwapchainKHR(context->device_handle, &swapchain_create_info, NULL, &swapchain->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
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

    return true;
}

Bool create_descriptor_set(VulkanContext *context, OUT VkDescriptorSetLayout *descriptor_set_layout, OUT VkDescriptorSet *descriptor_set)
{
    VkResult result_code;

    return true;
}

struct VulkanPipeline
{
    VkRenderPass render_pass;
    VkPipeline pipeline;
    Array<VkFramebuffer> framebuffers;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSet descriptor_set;
};

struct Vertex
{
    Real position[2];
    Real color[3];
};

Bool create_vulkan_shader(VulkanContext *context, RawStr filename, OUT VkShaderModule *shader_module)
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

Bool create_vulkan_pipeline(VulkanContext *context, VulkanSwapchain *swapchain, OUT VulkanPipeline *pipeline_data)
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

    result_code = vkCreateRenderPass(context->device_handle, &render_pass_create_info, NULL, &pipeline_data->render_pass);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkShaderModule vert_shader_module;
    if (!create_vulkan_shader(context, "vert.spv", &vert_shader_module))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = vert_shader_module;
    vertex_shader_stage_create_info.pName = "main";

    VkShaderModule frag_shader_module;
    if (!create_vulkan_shader(context, "frag.spv", &frag_shader_module))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_create_info.module = frag_shader_module;
    fragment_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stage_create_info[2] = {vertex_shader_stage_create_info, fragment_shader_stage_create_info};

    VkVertexInputBindingDescription vertex_binding_description = {};
    vertex_binding_description.binding = 0;
    vertex_binding_description.stride = sizeof(Vec3);
    vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attribute_description;
    vertex_attribute_description.binding = 0;
    vertex_attribute_description.location = 0;
    vertex_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attribute_description.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 1;
    vertex_input_state_create_info.pVertexAttributeDescriptions = &vertex_attribute_description;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (Real)swapchain->width;
    viewport.height = (Real)swapchain->height;
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
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

    VkDescriptorSetLayoutBinding descriptor_set_binding = {};
    descriptor_set_binding.binding = 0;
    descriptor_set_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_binding.descriptorCount = 1;
    descriptor_set_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptor_set_layout_create_info.bindingCount = 1;
    descriptor_set_layout_create_info.pBindings = &descriptor_set_binding;

    VkDescriptorSetLayout descriptor_set_layout;
    result_code = vkCreateDescriptorSetLayout(context->device_handle, &descriptor_set_layout_create_info, NULL, &descriptor_set_layout);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    descriptor_set_alloc_info.descriptorPool = context->descriptor_pool;
    descriptor_set_alloc_info.descriptorSetCount = 1;
    descriptor_set_alloc_info.pSetLayouts = &descriptor_set_layout;

    result_code = vkAllocateDescriptorSets(context->device_handle, &descriptor_set_alloc_info, &pipeline_data->descriptor_set);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = NULL;

    result_code = vkCreatePipelineLayout(context->device_handle, &pipeline_layout_create_info, NULL, &pipeline_data->pipeline_layout);

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
    pipeline_create_info.layout = pipeline_data->pipeline_layout;
    pipeline_create_info.renderPass = pipeline_data->render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    result_code = vkCreateGraphicsPipelines(context->device_handle, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline_data->pipeline);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    pipeline_data->framebuffers = create_array<VkFramebuffer>(swapchain->images.length);
    for (Int image_index = 0; image_index < swapchain->images.length; image_index++)
    {
        VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        image_view_create_info.image = swapchain->images[image_index];
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

        VkImageView image_view;
        result_code = vkCreateImageView(context->device_handle, &image_view_create_info, NULL, &image_view);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }

        VkFramebufferCreateInfo frame_buffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        frame_buffer_create_info.renderPass = pipeline_data->render_pass;
        frame_buffer_create_info.attachmentCount = 1;
        frame_buffer_create_info.pAttachments = &image_view;
        frame_buffer_create_info.width = swapchain->width;
        frame_buffer_create_info.height = swapchain->height;
        frame_buffer_create_info.layers = 1;

        VkFramebuffer *framebuffer = pipeline_data->framebuffers.push();
        result_code = vkCreateFramebuffer(context->device_handle, &frame_buffer_create_info, NULL, framebuffer);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

struct VulkanFrame
{
    VkCommandBuffer command_buffer;
    VkSemaphore image_aquired_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence frame_finished_fence;
};

Bool create_vulkan_semaphore(VulkanContext *context, OUT VkSemaphore *semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkResult result_code = vkCreateSemaphore(context->device_handle, &semaphore_create_info, NULL, semaphore);
    return result_code == VK_SUCCESS;
}

Bool create_vulkan_fence(VulkanContext *context, Bool signaled, OUT VkFence *fence)
{
    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_create_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u;

    VkResult result_code = vkCreateFence(context->device_handle, &fence_create_info, NULL, fence);
    return result_code == VK_SUCCESS;
}

Bool create_vulkan_frame(VulkanContext *context, Int frame_count, OUT Array<VulkanFrame> *frames)
{
    VkResult result_code;

    *frames = create_array<VulkanFrame>(frame_count);
    for (Int frame_index = 0; frame_index < frame_count; frame_index++)
    {
        VulkanFrame *frame = frames->push();

        VkCommandBufferAllocateInfo command_buffer_allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        command_buffer_allocate_info.commandPool = context->command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1;

        result_code = vkAllocateCommandBuffers(context->device_handle, &command_buffer_allocate_info, &frame->command_buffer);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }

        if (!create_vulkan_semaphore(context, &frame->image_aquired_semaphore))
        {
            return false;
        }

        if (!create_vulkan_semaphore(context, &frame->render_finished_semaphore))
        {
            return false;
        }

        if (!create_vulkan_fence(context, true, &frame->frame_finished_fence))
        {
            return false;
        }
    }

    return true;
}

struct VulkanBuffer
{
    VkBuffer handle;
    VkDeviceMemory memory;
    Int length;
    Int1 *data;
};

Bool create_vulkan_buffer(VulkanContext *context, Int length, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_property, VulkanBuffer *buffer)
{
    VkResult result_code;

    buffer->length = length;
    VkBufferCreateInfo buffer_create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_create_info.size = buffer->length;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result_code = vkCreateBuffer(context->device_handle, &buffer_create_info, NULL, &buffer->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(context->device_handle, buffer->handle, &memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context->physical_device_handle, &memory_properties);

    VkDeviceMemory buffer_memory;
    for (Int memory_type_index = 0; memory_type_index < memory_properties.memoryTypeCount; memory_type_index++)
    {
        if (has_flag(memory_requirements.memoryTypeBits, 1 << memory_type_index) &&
            has_flag(memory_properties.memoryTypes[memory_type_index].propertyFlags, memory_property))
        {
            VkMemoryAllocateInfo buffer_memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            buffer_memory_allocate_info.allocationSize = memory_requirements.size;
            buffer_memory_allocate_info.memoryTypeIndex = memory_type_index;

            result_code = vkAllocateMemory(context->device_handle, &buffer_memory_allocate_info, NULL, &buffer_memory);
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

    result_code = vkBindBufferMemory(context->device_handle, buffer->handle, buffer_memory, 0);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    if (has_flag(memory_property, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
    {
        result_code = vkMapMemory(context->device_handle, buffer_memory, 0, buffer_create_info.size, 0, (void **)&buffer->data);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }
    }
    else
    {
        buffer->data = NULL;
    }

    return true;
}

Bool render_vulkan_frame(VulkanContext *context, VulkanSwapchain *swapchain, VulkanPipeline *pipeline, VulkanFrame *frame,
                         VulkanBuffer *host_vertex_buffer, VulkanBuffer *vertex_buffer,
                         VulkanBuffer *host_index_buffer, VulkanBuffer *index_buffer,
                         VulkanBuffer *host_uniform_buffer, VulkanBuffer *uniform_buffer,
                         Int indices_count)
{
    VkResult result_code;
    result_code = vkWaitForFences(context->device_handle, 1, &frame->frame_finished_fence, false, UINT64_MAX);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    result_code = vkResetFences(context->device_handle, 1, &frame->frame_finished_fence);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    Int image_index;
    result_code = vkAcquireNextImageKHR(context->device_handle, swapchain->handle, UINT64_MAX, frame->image_aquired_semaphore, VK_NULL_HANDLE, (UInt4 *)&image_index);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferBeginInfo command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result_code = vkBeginCommandBuffer(frame->command_buffer, &command_buffer_begin_info);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkBufferCopy vertex_buffer_copy = {};
    vertex_buffer_copy.srcOffset = 0;
    vertex_buffer_copy.dstOffset = 0;
    vertex_buffer_copy.size = vertex_buffer->length;
    vkCmdCopyBuffer(frame->command_buffer, host_vertex_buffer->handle, vertex_buffer->handle, 1, &vertex_buffer_copy);

    VkBufferCopy index_buffer_copy = {};
    index_buffer_copy.srcOffset = 0;
    index_buffer_copy.dstOffset = 0;
    index_buffer_copy.size = index_buffer->length;
    vkCmdCopyBuffer(frame->command_buffer, host_index_buffer->handle, index_buffer->handle, 1, &index_buffer_copy);

    VkBufferCopy uniform_buffer_copy = {};
    uniform_buffer_copy.srcOffset = 0;
    uniform_buffer_copy.dstOffset = 0;
    uniform_buffer_copy.size = uniform_buffer->length;
    vkCmdCopyBuffer(frame->command_buffer, host_uniform_buffer->handle, uniform_buffer->handle, 1, &uniform_buffer_copy);

    VkBufferMemoryBarrier vertex_buffer_memory_barrier[2] = {{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER}, {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER}};
    vertex_buffer_memory_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vertex_buffer_memory_barrier[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    vertex_buffer_memory_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vertex_buffer_memory_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vertex_buffer_memory_barrier[0].buffer = vertex_buffer->handle;
    vertex_buffer_memory_barrier[0].offset = 0;
    vertex_buffer_memory_barrier[0].size = VK_WHOLE_SIZE;

    vertex_buffer_memory_barrier[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vertex_buffer_memory_barrier[1].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    vertex_buffer_memory_barrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vertex_buffer_memory_barrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vertex_buffer_memory_barrier[1].buffer = index_buffer->handle;
    vertex_buffer_memory_barrier[1].offset = 0;
    vertex_buffer_memory_barrier[1].size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 2, vertex_buffer_memory_barrier, 0, NULL);

    VkBufferMemoryBarrier uniform_buffer_memory_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    uniform_buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    uniform_buffer_memory_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
    uniform_buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uniform_buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uniform_buffer_memory_barrier.buffer = uniform_buffer->handle;
    uniform_buffer_memory_barrier.offset = 0;
    uniform_buffer_memory_barrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, NULL, 1, &uniform_buffer_memory_barrier, 0, NULL);

    VkClearValue clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
    VkRenderPassBeginInfo render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render_pass_begin_info.renderPass = pipeline->render_pass;
    render_pass_begin_info.framebuffer = pipeline->framebuffers[image_index];
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = {(UInt4)swapchain->width, (UInt4)swapchain->height};
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(frame->command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(frame->command_buffer, 0, 1, &vertex_buffer->handle, &offset);
    vkCmdBindIndexBuffer(frame->command_buffer, index_buffer->handle, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline_layout, 0, 1, &pipeline->descriptor_set, 0, NULL);

    vkCmdDrawIndexed(frame->command_buffer, indices_count, 1, 0, 0, 0);

    vkCmdEndRenderPass(frame->command_buffer);

    result_code = vkEndCommandBuffer(frame->command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &frame->image_aquired_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &frame->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &frame->render_finished_semaphore;

    result_code = vkQueueSubmit(context->graphics_queue, 1, &submit_info, frame->frame_finished_fence);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &frame->render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = (UInt4 *)&image_index,
    present_info.pResults = NULL;

    result_code = vkQueuePresentKHR(context->present_queue, &present_info);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}
