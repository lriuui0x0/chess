#pragma once

#include "util.cpp"
#include "math.cpp"
#include "window.cpp"
#include "asset.cpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

struct VulkanContext
{
    VkInstance instance_handle;
    VkPhysicalDevice physical_device_handle;
    VkDevice device_handle;
    UInt32 graphics_queue_index;
    VkQueue graphics_queue;
    VkSurfaceKHR surface_handle;
    UInt32 present_queue_index;
    VkQueue present_queue;
    VkCommandPool command_pool;
    VkCommandBuffer temp_command_buffer;
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
                                              Void *pUserData)
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

    UInt32 physical_devices_count = 0;
    result_code = vkEnumeratePhysicalDevices(context->instance_handle, &physical_devices_count, NULL);
    if (result_code != VK_SUCCESS || physical_devices_count < 1)
    {
        return false;
    }

    UInt32 desired_physical_devices_count = 1;
    result_code = vkEnumeratePhysicalDevices(context->instance_handle, &desired_physical_devices_count, &context->physical_device_handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(context->physical_device_handle, &physical_device_properties);

    VkWin32SurfaceCreateInfoKHR surface_create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    surface_create_info.hinstance = GetModuleHandleA(NULL);
    surface_create_info.hwnd = (HWND)window;

    result_code = vkCreateWin32SurfaceKHR(context->instance_handle, &surface_create_info, NULL, &context->surface_handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    UInt32 queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device_handle, &queue_families_count, NULL);

    VkQueueFamilyProperties queue_families_properties[8];
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device_handle, &queue_families_count, queue_families_properties);

    Bool found_graphics_queue = false;
    for (Int queue_family_index = 0; queue_family_index < (Int)queue_families_count; queue_family_index++)
    {
        if (queue_families_properties[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            context->graphics_queue_index = queue_family_index;
            found_graphics_queue = true;
            break;
        }
    }

    if (!found_graphics_queue)
    {
        return false;
    }

    Bool found_present_queue = false;
    for (Int queue_family_index = 0; queue_family_index < (Int)queue_families_count; queue_family_index++)
    {
        VkBool32 is_supported;
        result_code = vkGetPhysicalDeviceSurfaceSupportKHR(context->physical_device_handle, queue_family_index, context->surface_handle, &is_supported);
        if (result_code == VK_SUCCESS && is_supported)
        {
            context->present_queue_index = queue_family_index;
            found_present_queue = true;
            break;
        }
    }

    if (!found_present_queue)
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
    device_create_info.queueCreateInfoCount = (UInt32)queues_create_count;
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

    VkCommandBufferAllocateInfo command_buffer_alloc_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_buffer_alloc_info.commandPool = context->command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    result_code = vkAllocateCommandBuffers(context->device_handle, &command_buffer_alloc_info, &context->temp_command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorPoolSize descriptor_pool_size[2];
    descriptor_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_size[0].descriptorCount = 48;

    descriptor_pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_size[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptor_pool_create_info.maxSets = 49;
    descriptor_pool_create_info.poolSizeCount = 2;
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_size;

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
    swapchain_create_info.imageExtent.width = (UInt32)width;
    swapchain_create_info.imageExtent.height = (UInt32)height;
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

    UInt32 swapchain_images_count = 0;
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

struct VulkanScenePipeline
{
    VkPipeline handle;
    VkRenderPass render_pass;
    VkDescriptorSetLayout entity_descriptor_set_layout;
    VkDescriptorSetLayout scene_descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
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
    shader_module_create_info.pCode = (UInt32 *)code.data;

    VkResult result_code = vkCreateShaderModule(context->device_handle, &shader_module_create_info, NULL, shader_module);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool create_vulkan_scene_pipeline(VulkanContext *context, VulkanSwapchain *swapchain, OUT VulkanScenePipeline *scene_pipeline)
{
    VkResult result_code;

    VkAttachmentDescription framebuffer_description[3] = {};
    framebuffer_description[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    framebuffer_description[0].samples = VK_SAMPLE_COUNT_16_BIT;
    framebuffer_description[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    framebuffer_description[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    framebuffer_description[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    framebuffer_description[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    framebuffer_description[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    framebuffer_description[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    framebuffer_description[1].format = VK_FORMAT_D32_SFLOAT;
    framebuffer_description[1].samples = VK_SAMPLE_COUNT_16_BIT;
    framebuffer_description[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    framebuffer_description[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    framebuffer_description[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    framebuffer_description[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    framebuffer_description[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    framebuffer_description[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    framebuffer_description[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    framebuffer_description[2].samples = VK_SAMPLE_COUNT_1_BIT;
    framebuffer_description[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    framebuffer_description[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    framebuffer_description[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    framebuffer_description[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    framebuffer_description[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    framebuffer_description[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_framebuffer_reference = {};
    color_framebuffer_reference.attachment = 0;
    color_framebuffer_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_framebuffer_reference = {};
    depth_framebuffer_reference.attachment = 1;
    depth_framebuffer_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolve_framebuffer_reference = {};
    resolve_framebuffer_reference.attachment = 2;
    resolve_framebuffer_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_framebuffer_reference;
    subpass.pDepthStencilAttachment = &depth_framebuffer_reference;
    subpass.pResolveAttachments = &resolve_framebuffer_reference;

    VkRenderPassCreateInfo render_pass_create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_create_info.attachmentCount = 3;
    render_pass_create_info.pAttachments = framebuffer_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    result_code = vkCreateRenderPass(context->device_handle, &render_pass_create_info, NULL, &scene_pipeline->render_pass);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkShaderModule vert_shader_module;
    if (!create_vulkan_shader(context, "scene.vert.spv", &vert_shader_module))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = vert_shader_module;
    vertex_shader_stage_create_info.pName = "main";

    VkShaderModule frag_shader_module;
    if (!create_vulkan_shader(context, "scene.frag.spv", &frag_shader_module))
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
    vertex_binding_description.stride = sizeof(Vertex);
    vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attribute_description[3];
    vertex_attribute_description[0].binding = 0;
    vertex_attribute_description[0].location = 0;
    vertex_attribute_description[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attribute_description[0].offset = offsetof(Vertex, pos);

    vertex_attribute_description[1].binding = 0;
    vertex_attribute_description[1].location = 1;
    vertex_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attribute_description[1].offset = offsetof(Vertex, normal);

    vertex_attribute_description[2].binding = 0;
    vertex_attribute_description[2].location = 2;
    vertex_attribute_description[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attribute_description[2].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 3;
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_description;

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
    scissor.extent = {(UInt32)swapchain->width, (UInt32)swapchain->height};

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_create_info.depthBiasClamp = 0.0f;
    rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
    rasterization_state_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multi_sample_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multi_sample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_16_BIT;
    multi_sample_state_create_info.sampleShadingEnable = VK_FALSE;
    multi_sample_state_create_info.minSampleShading = 1.0f;
    multi_sample_state_create_info.pSampleMask = NULL;
    multi_sample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multi_sample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front = {};
    depth_stencil_state_create_info.back = {};
    depth_stencil_state_create_info.minDepthBounds = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds = 1.0f;

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

    result_code = vkCreateDescriptorSetLayout(context->device_handle, &descriptor_set_layout_create_info, NULL, &scene_pipeline->entity_descriptor_set_layout);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    result_code = vkCreateDescriptorSetLayout(context->device_handle, &descriptor_set_layout_create_info, NULL, &scene_pipeline->scene_descriptor_set_layout);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorSetLayout descriptor_set_layout[2];
    descriptor_set_layout[0] = scene_pipeline->entity_descriptor_set_layout;
    descriptor_set_layout[1] = scene_pipeline->scene_descriptor_set_layout;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_create_info.setLayoutCount = 2;
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layout;

    result_code = vkCreatePipelineLayout(context->device_handle, &pipeline_layout_create_info, NULL, &scene_pipeline->pipeline_layout);

    VkGraphicsPipelineCreateInfo pipeline_create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stage_create_info;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pTessellationState = NULL;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multi_sample_state_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = NULL;
    pipeline_create_info.layout = scene_pipeline->pipeline_layout;
    pipeline_create_info.renderPass = scene_pipeline->render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    result_code = vkCreateGraphicsPipelines(context->device_handle, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &scene_pipeline->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

struct VulkanDebugUIPipeline
{
    VkPipeline handle;
    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout font_texture_descriptor_set_layout;
};

struct DebugUIVertex
{
    Vec2 pos;
    Vec2 texture_coord;
};

Bool create_vulkan_debug_ui_pipeline(VulkanContext *context, VulkanSwapchain *swapchain, OUT VulkanDebugUIPipeline *debug_ui_pipeline)
{
    VkResult result_code;

    VkAttachmentDescription framebuffer_description[1] = {};
    framebuffer_description[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    framebuffer_description[0].samples = VK_SAMPLE_COUNT_1_BIT;
    framebuffer_description[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    framebuffer_description[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    framebuffer_description[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    framebuffer_description[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    framebuffer_description[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    framebuffer_description[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_framebuffer_reference = {};
    color_framebuffer_reference.attachment = 0;
    color_framebuffer_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_framebuffer_reference;

    VkRenderPassCreateInfo render_pass_create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = framebuffer_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    result_code = vkCreateRenderPass(context->device_handle, &render_pass_create_info, NULL, &debug_ui_pipeline->render_pass);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkShaderModule vert_shader_module;
    if (!create_vulkan_shader(context, "debug_ui.vert.spv", &vert_shader_module))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = vert_shader_module;
    vertex_shader_stage_create_info.pName = "main";

    VkShaderModule frag_shader_module;
    if (!create_vulkan_shader(context, "debug_ui.frag.spv", &frag_shader_module))
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
    vertex_binding_description.stride = sizeof(DebugUIVertex);
    vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attribute_description[2];
    vertex_attribute_description[0].binding = 0;
    vertex_attribute_description[0].location = 0;
    vertex_attribute_description[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_attribute_description[0].offset = offsetof(DebugUIVertex, pos);

    vertex_attribute_description[1].binding = 0;
    vertex_attribute_description[1].location = 1;
    vertex_attribute_description[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_attribute_description[1].offset = offsetof(DebugUIVertex, texture_coord);

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_description;

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
    scissor.extent = {(UInt32)swapchain->width, (UInt32)swapchain->height};

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
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

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_stencil_state_create_info.depthTestEnable = VK_FALSE;
    depth_stencil_state_create_info.depthWriteEnable = VK_FALSE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front = {};
    depth_stencil_state_create_info.back = {};
    depth_stencil_state_create_info.minDepthBounds = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState framebuffer_color_blend_state;
    framebuffer_color_blend_state.blendEnable = VK_TRUE;
    framebuffer_color_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    framebuffer_color_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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
    descriptor_set_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_binding.descriptorCount = 1;
    descriptor_set_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptor_set_layout_create_info.bindingCount = 1;
    descriptor_set_layout_create_info.pBindings = &descriptor_set_binding;

    result_code = vkCreateDescriptorSetLayout(context->device_handle, &descriptor_set_layout_create_info, NULL, &debug_ui_pipeline->font_texture_descriptor_set_layout);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorSetLayout descriptor_set_layout[1];
    descriptor_set_layout[0] = debug_ui_pipeline->font_texture_descriptor_set_layout;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layout;

    result_code = vkCreatePipelineLayout(context->device_handle, &pipeline_layout_create_info, NULL, &debug_ui_pipeline->pipeline_layout);

    VkGraphicsPipelineCreateInfo pipeline_create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stage_create_info;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pTessellationState = NULL;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multi_sample_state_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = NULL;
    pipeline_create_info.layout = debug_ui_pipeline->pipeline_layout;
    pipeline_create_info.renderPass = debug_ui_pipeline->render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    result_code = vkCreateGraphicsPipelines(context->device_handle, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &debug_ui_pipeline->handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

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

struct VulkanBuffer
{
    VkBuffer handle;
    VkDeviceMemory memory;
    Int length;
    Int8 *data;
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

    VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
    for (Int memory_type_index = 0; memory_type_index < (Int)memory_properties.memoryTypeCount; memory_type_index++)
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

    if (buffer_memory == VK_NULL_HANDLE)
    {
        return false;
    }

    result_code = vkBindBufferMemory(context->device_handle, buffer->handle, buffer_memory, 0);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    if (has_flag(memory_property, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
    {
        result_code = vkMapMemory(context->device_handle, buffer_memory, 0, buffer_create_info.size, 0, (Void **)&buffer->data);
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

Bool create_vulkan_image(VulkanContext *context, Int width, Int height, VkFormat format, VkSampleCountFlagBits sample_count, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_property, OUT VkImage *image)
{
    VkResult result_code;

    VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.extent = {(UInt32)width, (UInt32)height, 1};
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = sample_count;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    result_code = vkCreateImage(context->device_handle, &image_create_info, NULL, image);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(context->device_handle, *image, &memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context->physical_device_handle, &memory_properties);

    VkDeviceMemory image_memory = VK_NULL_HANDLE;
    for (Int memory_type_index = 0; memory_type_index < (Int)memory_properties.memoryTypeCount; memory_type_index++)
    {
        if (has_flag(memory_requirements.memoryTypeBits, 1 << memory_type_index) &&
            has_flag(memory_properties.memoryTypes[memory_type_index].propertyFlags, memory_property))
        {
            VkMemoryAllocateInfo image_memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            image_memory_allocate_info.allocationSize = memory_requirements.size;
            image_memory_allocate_info.memoryTypeIndex = memory_type_index;

            result_code = vkAllocateMemory(context->device_handle, &image_memory_allocate_info, NULL, &image_memory);
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

    result_code = vkBindImageMemory(context->device_handle, *image, image_memory, 0);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool create_image_view(VulkanContext *context, VkImage image, VkFormat format, VkImageAspectFlags aspect_mask, OUT VkImageView *image_view)
{
    VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
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

    VkResult result_code = vkCreateImageView(context->device_handle, &image_view_create_info, NULL, image_view);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool upload_vulkan_buffer(VulkanContext *context, VulkanBuffer *host_buffer, VulkanBuffer *device_buffer)
{
    VkResult result_code;

    VkCommandBufferBeginInfo command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result_code = vkBeginCommandBuffer(context->temp_command_buffer, &command_buffer_begin_info);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkBufferCopy buffer_copy = {};
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = host_buffer->length;
    vkCmdCopyBuffer(context->temp_command_buffer, host_buffer->handle, device_buffer->handle, 1, &buffer_copy);

    result_code = vkEndCommandBuffer(context->temp_command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &context->temp_command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    result_code = vkQueueSubmit(context->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    result_code = vkDeviceWaitIdle(context->device_handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

Bool upload_vulkan_texture(VulkanContext *context, VulkanBuffer *host_buffer, VkImage image, Int width, Int height)
{
    VkResult result_code;

    VkCommandBufferBeginInfo command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result_code = vkBeginCommandBuffer(context->temp_command_buffer, &command_buffer_begin_info);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkImageMemoryBarrier image_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
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
    vkCmdPipelineBarrier(context->temp_command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &image_barrier);

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

    vkCmdCopyBufferToImage(context->temp_command_buffer, host_buffer->handle, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

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
    vkCmdPipelineBarrier(context->temp_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &image_barrier);

    result_code = vkEndCommandBuffer(context->temp_command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &context->temp_command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    result_code = vkQueueSubmit(context->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    result_code = vkDeviceWaitIdle(context->device_handle);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}

struct VulkanSceneFrame
{
    VkCommandBuffer command_buffer;
    VkSemaphore image_aquired_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence frame_finished_fence;
    Array<VkFramebuffer> frame_buffers;
    VkImage color_image;
    VkImage depth_image;
    VulkanBuffer vertex_buffer;
    VulkanBuffer index_buffer;
    VulkanBuffer uniform_buffer;
    Array<VkDescriptorSet> entity_descriptor_sets;
    VkDescriptorSet scene_descriptor_set;
};

struct Entity
{
    Str name;
    Vec3 pos;
    Mat4 rotation;
    Mesh *mesh;
};

struct Scene
{
    Mat4 view;
    Mat4 normal_view;
    Mat4 projection;
    Vec4 light_dir[4];
};

struct Piece
{
    Mat4 world;
    Mat4 normal_world;
    Vec4 color;
    Vec4 padding;
};

Bool create_vulkan_scene_frame(VulkanContext *context, VulkanSwapchain *swapchain, VulkanScenePipeline *scene_pipeline, Array<Entity> *entities,
                               OUT VulkanSceneFrame *scene_frame, OUT VulkanBuffer *host_vertex_buffer, OUT VulkanBuffer *host_index_buffer, OUT VulkanBuffer *host_uniform_buffer)
{
    VkResult result_code;

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_buffer_allocate_info.commandPool = context->command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    result_code = vkAllocateCommandBuffers(context->device_handle, &command_buffer_allocate_info, &scene_frame->command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    if (!create_vulkan_semaphore(context, &scene_frame->image_aquired_semaphore))
    {
        return false;
    }

    if (!create_vulkan_semaphore(context, &scene_frame->render_finished_semaphore))
    {
        return false;
    }

    if (!create_vulkan_fence(context, true, &scene_frame->frame_finished_fence))
    {
        return false;
    }

    if (!create_vulkan_image(context, swapchain->width, swapchain->height,
                             VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_16_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scene_frame->color_image))
    {
        return false;
    }

    VkImageView color_image_view;
    if (!create_image_view(context, scene_frame->color_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &color_image_view))
    {
        return false;
    }

    if (!create_vulkan_image(context, swapchain->width, swapchain->height,
                             VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_16_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scene_frame->depth_image))
    {
        return false;
    }

    VkImageView depth_image_view;
    if (!create_image_view(context, scene_frame->depth_image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_view))
    {
        return false;
    }

    scene_frame->frame_buffers = create_array<VkFramebuffer>(swapchain->images.length);
    for (Int image_index = 0; image_index < swapchain->images.length; image_index++)
    {
        VkImageView image_view;
        if (!create_image_view(context, swapchain->images[image_index], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &image_view))
        {
            return false;
        }

        VkImageView attachments[3] = {color_image_view, depth_image_view, image_view};
        VkFramebufferCreateInfo frame_buffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        frame_buffer_create_info.renderPass = scene_pipeline->render_pass;
        frame_buffer_create_info.attachmentCount = 3;
        frame_buffer_create_info.pAttachments = attachments;
        frame_buffer_create_info.width = swapchain->width;
        frame_buffer_create_info.height = swapchain->height;
        frame_buffer_create_info.layers = 1;

        VkFramebuffer *frame_buffer = scene_frame->frame_buffers.push();
        result_code = vkCreateFramebuffer(context->device_handle, &frame_buffer_create_info, NULL, frame_buffer);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }
    }

    Int total_vertex_data_length = 0;
    Int total_index_data_length = 0;
    for (Int i = 0; i < entities->length; i++)
    {
        total_vertex_data_length += sizeof(Vertex) * entities->data[i].mesh->vertex_count;
        total_index_data_length += sizeof(UInt32) * entities->data[i].mesh->index_count;
    }
    Int total_uniform_data_length = sizeof(Scene) + sizeof(Piece) * entities->length;

    if (!create_vulkan_buffer(context, total_vertex_data_length,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &scene_frame->vertex_buffer))
    {
        return false;
    }

    if (!create_vulkan_buffer(context, total_vertex_data_length,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              host_vertex_buffer))
    {
        return false;
    }

    if (!create_vulkan_buffer(context, total_index_data_length,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &scene_frame->index_buffer))
    {
        return false;
    }

    if (!create_vulkan_buffer(context, total_index_data_length,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              host_index_buffer))
    {
        return false;
    }

    if (!create_vulkan_buffer(context, total_uniform_data_length,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &scene_frame->uniform_buffer))
    {
        return false;
    }

    if (!create_vulkan_buffer(context, total_uniform_data_length,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              host_uniform_buffer))
    {
        return false;
    }

    VkDescriptorSetAllocateInfo common_descriptor_set_alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    common_descriptor_set_alloc_info.descriptorPool = context->descriptor_pool;
    common_descriptor_set_alloc_info.descriptorSetCount = 1;
    common_descriptor_set_alloc_info.pSetLayouts = &scene_pipeline->scene_descriptor_set_layout;

    result_code = vkAllocateDescriptorSets(context->device_handle, &common_descriptor_set_alloc_info, &scene_frame->scene_descriptor_set);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = scene_frame->uniform_buffer.handle;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = sizeof(Scene);

    VkWriteDescriptorSet descriptor_set_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor_set_write.dstSet = scene_frame->scene_descriptor_set;
    descriptor_set_write.dstBinding = 0;
    descriptor_set_write.dstArrayElement = 0;
    descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_write.descriptorCount = 1;
    descriptor_set_write.pBufferInfo = &descriptor_buffer_info;

    vkUpdateDescriptorSets(context->device_handle, 1, &descriptor_set_write, 0, NULL);

    scene_frame->entity_descriptor_sets = create_array<VkDescriptorSet>(entities->length);
    scene_frame->entity_descriptor_sets.length = entities->length;

    for (Int i = 0; i < entities->length; i++)
    {
        VkDescriptorSetAllocateInfo entity_descriptor_set_alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        entity_descriptor_set_alloc_info.descriptorPool = context->descriptor_pool;
        entity_descriptor_set_alloc_info.descriptorSetCount = 1;
        entity_descriptor_set_alloc_info.pSetLayouts = &scene_pipeline->entity_descriptor_set_layout;

        result_code = vkAllocateDescriptorSets(context->device_handle, &entity_descriptor_set_alloc_info, &scene_frame->entity_descriptor_sets[i]);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }

        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = scene_frame->uniform_buffer.handle;
        descriptor_buffer_info.offset = i * sizeof(Piece) + sizeof(Scene);
        descriptor_buffer_info.range = sizeof(Piece);

        VkWriteDescriptorSet descriptor_set_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor_set_write.dstSet = scene_frame->entity_descriptor_sets[i];
        descriptor_set_write.dstBinding = 0;
        descriptor_set_write.dstArrayElement = 0;
        descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_set_write.descriptorCount = 1;
        descriptor_set_write.pBufferInfo = &descriptor_buffer_info;

        vkUpdateDescriptorSets(context->device_handle, 1, &descriptor_set_write, 0, NULL);
    }

    return true;
}

struct VulkanDebugUIFrame
{
    VulkanBuffer vertex_buffer;
    Array<VkFramebuffer> frame_buffers;
    VkImage font_texture;
    VkSampler font_texture_sampler;
    VkDescriptorSet font_texture_descriptor_set;
};

Bool create_vulkan_debug_ui_frame(VulkanContext *context, VulkanSwapchain *swapchain, VulkanDebugUIPipeline *debug_ui_pipeline, Font *debug_font,
                                  OUT VulkanDebugUIFrame *debug_ui_frame, OUT VulkanBuffer *host_vertex_buffer)
{
    VkResult result_code;

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_buffer_allocate_info.commandPool = context->command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    debug_ui_frame->frame_buffers = create_array<VkFramebuffer>(swapchain->images.length);
    for (Int image_index = 0; image_index < swapchain->images.length; image_index++)
    {
        VkImageView image_view;
        if (!create_image_view(context, swapchain->images[image_index], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &image_view))
        {
            return false;
        }

        VkImageView attachments[1] = {image_view};
        VkFramebufferCreateInfo frame_buffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        frame_buffer_create_info.renderPass = debug_ui_pipeline->render_pass;
        frame_buffer_create_info.attachmentCount = 1;
        frame_buffer_create_info.pAttachments = attachments;
        frame_buffer_create_info.width = swapchain->width;
        frame_buffer_create_info.height = swapchain->height;
        frame_buffer_create_info.layers = 1;

        VkFramebuffer *frame_buffer = debug_ui_frame->frame_buffers.push();
        result_code = vkCreateFramebuffer(context->device_handle, &frame_buffer_create_info, NULL, frame_buffer);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }
    }

    Int max_letter_count = 1024;
    Int vertex_buffer_length = sizeof(DebugUIVertex) * max_letter_count;

    if (!create_vulkan_buffer(context, vertex_buffer_length,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &debug_ui_frame->vertex_buffer))
    {
        return false;
    }

    if (!create_vulkan_buffer(context, vertex_buffer_length,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              host_vertex_buffer))
    {
        return false;
    }

    if (!create_vulkan_image(context, debug_font->width, debug_font->height,
                             VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &debug_ui_frame->font_texture))
    {
        return false;
    }

    Int image_buffer_length = sizeof(debug_font->data[0]) * debug_font->width * debug_font->height;
    VulkanBuffer host_image_buffer;
    if (!create_vulkan_buffer(context, image_buffer_length,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &host_image_buffer))
    {
        return false;
    }

    memcpy(host_image_buffer.data, debug_font->data, image_buffer_length);

    if (!upload_vulkan_texture(context, &host_image_buffer, debug_ui_frame->font_texture, debug_font->width, debug_font->height))
    {
        return false;
    }

    VkImageView font_texture_view;
    if (!create_image_view(context, debug_ui_frame->font_texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &font_texture_view))
    {
        return false;
    }

    VkSamplerCreateInfo sampler_create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler_create_info.magFilter = VK_FILTER_NEAREST;
    sampler_create_info.minFilter = VK_FILTER_NEAREST;
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

    result_code = vkCreateSampler(context->device_handle, &sampler_create_info, NULL, &debug_ui_frame->font_texture_sampler);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorSetAllocateInfo debug_font_texture_descriptor_set_alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    debug_font_texture_descriptor_set_alloc_info.descriptorPool = context->descriptor_pool;
    debug_font_texture_descriptor_set_alloc_info.descriptorSetCount = 1;
    debug_font_texture_descriptor_set_alloc_info.pSetLayouts = &debug_ui_pipeline->font_texture_descriptor_set_layout;

    result_code = vkAllocateDescriptorSets(context->device_handle, &debug_font_texture_descriptor_set_alloc_info, &debug_ui_frame->font_texture_descriptor_set);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorImageInfo descriptor_image_info;
    descriptor_image_info.sampler = debug_ui_frame->font_texture_sampler;
    descriptor_image_info.imageView = font_texture_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptor_set_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor_set_write.dstSet = debug_ui_frame->font_texture_descriptor_set;
    descriptor_set_write.dstBinding = 0;
    descriptor_set_write.dstArrayElement = 0;
    descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_write.descriptorCount = 1;
    descriptor_set_write.pImageInfo = &descriptor_image_info;

    vkUpdateDescriptorSets(context->device_handle, 1, &descriptor_set_write, 0, NULL);

    return true;
}

Bool render_vulkan_frame(VulkanContext *context, VulkanSwapchain *swapchain,
                         VulkanScenePipeline *scene_pipeline, VulkanSceneFrame *scene_frame, Array<Entity> *entities, VulkanBuffer *host_uniform_buffer,
                         VulkanDebugUIPipeline *debug_ui_pipeline, VulkanDebugUIFrame *debug_ui_frame, Int debug_ui_letter_count)
{
    VkResult result_code;
    result_code = vkWaitForFences(context->device_handle, 1, &scene_frame->frame_finished_fence, false, UINT64_MAX);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    result_code = vkResetFences(context->device_handle, 1, &scene_frame->frame_finished_fence);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    Int image_index;
    result_code = vkAcquireNextImageKHR(context->device_handle, swapchain->handle, UINT64_MAX, scene_frame->image_aquired_semaphore, VK_NULL_HANDLE, (UInt32 *)&image_index);
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

    VkBufferCopy uniform_buffer_copy = {};
    uniform_buffer_copy.srcOffset = 0;
    uniform_buffer_copy.dstOffset = 0;
    uniform_buffer_copy.size = sizeof(Scene);
    vkCmdCopyBuffer(scene_frame->command_buffer, host_uniform_buffer->handle, scene_frame->uniform_buffer.handle, 1, &uniform_buffer_copy);

    VkBufferMemoryBarrier uniform_buffer_memory_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    uniform_buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    uniform_buffer_memory_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
    uniform_buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uniform_buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uniform_buffer_memory_barrier.buffer = scene_frame->uniform_buffer.handle;
    uniform_buffer_memory_barrier.offset = 0;
    uniform_buffer_memory_barrier.size = sizeof(Scene);

    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, NULL, 1, &uniform_buffer_memory_barrier, 0, NULL);

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

    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, NULL, 0, NULL, 1, &depth_image_memory_barrier);

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

    vkCmdPipelineBarrier(scene_frame->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &color_image_memory_barrier);

    // NOTE: Scene
    VkClearValue clear_colors[2] = {{0.7, 0.7, 0.7, 0.7},
                                    {1.0, 0}};
    VkRenderPassBeginInfo scene_render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    scene_render_pass_begin_info.renderPass = scene_pipeline->render_pass;
    scene_render_pass_begin_info.framebuffer = scene_frame->frame_buffers[image_index];
    scene_render_pass_begin_info.renderArea.offset = {0, 0};
    scene_render_pass_begin_info.renderArea.extent = {(UInt32)swapchain->width, (UInt32)swapchain->height};
    scene_render_pass_begin_info.clearValueCount = 2;
    scene_render_pass_begin_info.pClearValues = clear_colors;

    vkCmdBeginRenderPass(scene_frame->command_buffer, &scene_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->handle);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &scene_frame->vertex_buffer.handle, &offset);
    vkCmdBindIndexBuffer(scene_frame->command_buffer, scene_frame->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    Int index_offset = 0;
    Int vertex_offset = 0;
    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->pipeline_layout, 0, 1, &scene_frame->scene_descriptor_set, 0, NULL);
    for (Int entity_index = 0; entity_index < entities->length; entity_index++)
    {
        vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_pipeline->pipeline_layout, 1, 1, &scene_frame->entity_descriptor_sets[entity_index], 0, NULL);
        vkCmdDrawIndexed(scene_frame->command_buffer, entities->data[entity_index].mesh->index_count, 1, index_offset, vertex_offset, 0);
        index_offset += entities->data[entity_index].mesh->index_count;
        vertex_offset += entities->data[entity_index].mesh->vertex_count;
    }

    vkCmdEndRenderPass(scene_frame->command_buffer);

    // NOTE: Debug UI
    VkRenderPassBeginInfo debug_ui_render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    debug_ui_render_pass_begin_info.renderPass = debug_ui_pipeline->render_pass;
    debug_ui_render_pass_begin_info.framebuffer = debug_ui_frame->frame_buffers[image_index];
    debug_ui_render_pass_begin_info.renderArea.offset = {0, 0};
    debug_ui_render_pass_begin_info.renderArea.extent = {(UInt32)swapchain->width, (UInt32)swapchain->height};
    debug_ui_render_pass_begin_info.clearValueCount = 0;
    debug_ui_render_pass_begin_info.pClearValues = NULL;

    vkCmdBeginRenderPass(scene_frame->command_buffer, &debug_ui_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_ui_pipeline->handle);

    offset = 0;
    vkCmdBindVertexBuffers(scene_frame->command_buffer, 0, 1, &debug_ui_frame->vertex_buffer.handle, &offset);

    vkCmdBindDescriptorSets(scene_frame->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debug_ui_pipeline->pipeline_layout, 0, 1, &debug_ui_frame->font_texture_descriptor_set, 0, NULL);

    vkCmdDraw(scene_frame->command_buffer, debug_ui_letter_count * 2 * 3, 1, 0, 0);

    vkCmdEndRenderPass(scene_frame->command_buffer);

    result_code = vkEndCommandBuffer(scene_frame->command_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &scene_frame->image_aquired_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &scene_frame->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &scene_frame->render_finished_semaphore;

    result_code = vkQueueSubmit(context->graphics_queue, 1, &submit_info, scene_frame->frame_finished_fence);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &scene_frame->render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = (UInt32 *)&image_index,
    present_info.pResults = NULL;

    result_code = vkQueuePresentKHR(context->present_queue, &present_info);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}
