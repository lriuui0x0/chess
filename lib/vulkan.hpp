#pragma once

#include "util.hpp"

#define VK_NO_PROTOTYPES
// TODO: Platform indepedent
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define VK_FUNC_LIST_EXPORT \
    VK_FUNC(vkGetInstanceProcAddr)

#define VK_FUNC_LIST_GLOBAL   \
    VK_FUNC(vkCreateInstance) \
    VK_FUNC(vkEnumerateInstanceExtensionProperties)

// TODO: Platform independent surface creation function
#define VK_FUNC_LIST_INSTANCE                          \
    VK_FUNC(vkEnumeratePhysicalDevices)                \
    VK_FUNC(vkGetPhysicalDeviceProperties)             \
    VK_FUNC(vkGetPhysicalDeviceFeatures)               \
    VK_FUNC(vkGetPhysicalDeviceQueueFamilyProperties)  \
    VK_FUNC(vkGetPhysicalDeviceMemoryProperties)       \
    VK_FUNC(vkCreateDevice)                            \
    VK_FUNC(vkEnumerateDeviceExtensionProperties)      \
    VK_FUNC(vkGetDeviceProcAddr)                       \
    VK_FUNC(vkDestroyInstance)                         \
    VK_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR)      \
    VK_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    VK_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR)      \
    VK_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    VK_FUNC(vkCreateWin32SurfaceKHR)                   \
    VK_FUNC(vkDestroySurfaceKHR)                       \
    VK_FUNC(vkCreateDebugReportCallbackEXT)

#define VK_FUNC_LIST_DEVICE                \
    VK_FUNC(vkGetDeviceQueue)              \
    VK_FUNC(vkDeviceWaitIdle)              \
    VK_FUNC(vkDestroyDevice)               \
    VK_FUNC(vkCreateSemaphore)             \
    VK_FUNC(vkCreateCommandPool)           \
    VK_FUNC(vkAllocateCommandBuffers)      \
    VK_FUNC(vkBeginCommandBuffer)          \
    VK_FUNC(vkCmdPipelineBarrier)          \
    VK_FUNC(vkCmdClearColorImage)          \
    VK_FUNC(vkEndCommandBuffer)            \
    VK_FUNC(vkQueueSubmit)                 \
    VK_FUNC(vkFreeCommandBuffers)          \
    VK_FUNC(vkDestroyCommandPool)          \
    VK_FUNC(vkDestroySemaphore)            \
    VK_FUNC(vkCreateImageView)             \
    VK_FUNC(vkCreateRenderPass)            \
    VK_FUNC(vkCreateFramebuffer)           \
    VK_FUNC(vkCreateShaderModule)          \
    VK_FUNC(vkCreatePipelineLayout)        \
    VK_FUNC(vkCreateGraphicsPipelines)     \
    VK_FUNC(vkCmdBeginRenderPass)          \
    VK_FUNC(vkCmdBindPipeline)             \
    VK_FUNC(vkCmdDraw)                     \
    VK_FUNC(vkCmdBindIndexBuffer)          \
    VK_FUNC(vkCmdDrawIndexed)              \
    VK_FUNC(vkCmdEndRenderPass)            \
    VK_FUNC(vkDestroyShaderModule)         \
    VK_FUNC(vkDestroyPipelineLayout)       \
    VK_FUNC(vkDestroyPipeline)             \
    VK_FUNC(vkDestroyRenderPass)           \
    VK_FUNC(vkDestroyFramebuffer)          \
    VK_FUNC(vkDestroyImageView)            \
    VK_FUNC(vkCreateFence)                 \
    VK_FUNC(vkCreateBuffer)                \
    VK_FUNC(vkGetBufferMemoryRequirements) \
    VK_FUNC(vkAllocateMemory)              \
    VK_FUNC(vkBindBufferMemory)            \
    VK_FUNC(vkMapMemory)                   \
    VK_FUNC(vkFlushMappedMemoryRanges)     \
    VK_FUNC(vkUnmapMemory)                 \
    VK_FUNC(vkCmdSetViewport)              \
    VK_FUNC(vkCmdSetScissor)               \
    VK_FUNC(vkCmdBindVertexBuffers)        \
    VK_FUNC(vkWaitForFences)               \
    VK_FUNC(vkResetFences)                 \
    VK_FUNC(vkFreeMemory)                  \
    VK_FUNC(vkDestroyBuffer)               \
    VK_FUNC(vkDestroyFence)                \
    VK_FUNC(vkCmdCopyBuffer)               \
    VK_FUNC(vkCreateImage)                 \
    VK_FUNC(vkGetImageMemoryRequirements)  \
    VK_FUNC(vkBindImageMemory)             \
    VK_FUNC(vkCreateSampler)               \
    VK_FUNC(vkCmdCopyBufferToImage)        \
    VK_FUNC(vkCreateDescriptorSetLayout)   \
    VK_FUNC(vkCreateDescriptorPool)        \
    VK_FUNC(vkAllocateDescriptorSets)      \
    VK_FUNC(vkUpdateDescriptorSets)        \
    VK_FUNC(vkCmdBindDescriptorSets)       \
    VK_FUNC(vkDestroyDescriptorPool)       \
    VK_FUNC(vkDestroyDescriptorSetLayout)  \
    VK_FUNC(vkDestroySampler)              \
    VK_FUNC(vkDestroyImage)                \
    VK_FUNC(vkCreateSwapchainKHR)          \
    VK_FUNC(vkGetSwapchainImagesKHR)       \
    VK_FUNC(vkAcquireNextImageKHR)         \
    VK_FUNC(vkQueuePresentKHR)             \
    VK_FUNC(vkDestroySwapchainKHR)

#undef VK_FUNC
#define VK_FUNC(func_name) PFN_##func_name func_name;
VK_FUNC_LIST_EXPORT
VK_FUNC_LIST_GLOBAL
VK_FUNC_LIST_INSTANCE
VK_FUNC_LIST_DEVICE

#define MAX_SWAPCHAIN_IMAGE_COUNT (4)

struct VulkanSwapchain
{
    VkSwapchainKHR handle;
    InlineBuffer<VkImage, MAX_SWAPCHAIN_IMAGE_COUNT> images;
    Int width;
    Int height;
    VkFormat format;
};

struct VulkanDevice
{
    VkDevice handle;
    VulkanSwapchain swapchain;
    VkQueue graphics_queue;
    VkQueue present_queue;
    Int graphics_queue_index;
    Int present_queue_index;
    VkCommandPool command_pool;
    VkDescriptorPool descriptor_pool;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceFeatures physical_device_features;
    VkPhysicalDeviceProperties physical_device_properties;
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    VkDebugReportCallbackEXT debug_callback;
};

struct ShaderInfo
{
    Str code;
    VkShaderStageFlagBits stage;
};

struct VertexAttributeInfo
{
    Int offset;
    Int count;
};

struct DescriptorBindingInfo
{
    VkDescriptorType type;
    VkShaderStageFlagBits stage;
};

struct DescriptorSetInfo
{
    Buffer<DescriptorBindingInfo> bindings;
};

struct AttachmentInfo
{
    VkFormat format;
    VkSampleCountFlagBits multisample_count;
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
    VkImageLayout initial_layout;
    VkImageLayout working_layout;
    VkImageLayout final_layout;
};

#define MAX_DESCRIPTOR_SET_COUNT (8)

struct VulkanPipeline
{
    VkPipeline handle;
    VkRenderPass render_pass;
    VkPipelineLayout layout;
    InlineBuffer<VkDescriptorSetLayout, MAX_DESCRIPTOR_SET_COUNT> descriptor_set_layouts;
};

struct VulkanBuffer
{
    VkBuffer handle;
    VkDeviceMemory memory;
    Int count;
    UInt8 *data;
};

typedef Void (*VulkanDebugCallback)(Str message);

Bool create_device(Handle window, VulkanDebugCallback debug_callback, VulkanDevice *device);
Bool create_swapchain(VulkanDevice *device, Int width, Int height, Int desired_image_count, VkPresentModeKHR desired_present_mode);
Bool create_render_pass(VulkanDevice *device, Buffer<AttachmentInfo> *color_attachments, AttachmentInfo *depth_attachment, AttachmentInfo *resolve_attachment, VkRenderPass *render_pass);
Bool create_pipeline(VulkanDevice *device,
                     VkRenderPass render_pass, Int subpass,
                     Buffer<ShaderInfo> *shaders,
                     Int vertex_stride, Buffer<VertexAttributeInfo> *vertex_attributes, VkPrimitiveTopology primitive_type,
                     Buffer<DescriptorSetInfo> *descriptor_sets,
                     VkSampleCountFlagBits multisample_count, Bool depth_enable, Bool alpha_blend_enable,
                     VulkanPipeline *pipeline);

Bool create_semaphore(VulkanDevice *device, VkSemaphore *semaphore);
Bool create_fence(VulkanDevice *device, Bool signaled, VkFence *fence);
Bool create_buffer(VulkanDevice *device, Int count, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_property, VulkanBuffer *buffer);
Bool create_image(VulkanDevice *device, Int width, Int height, VkFormat format, VkSampleCountFlagBits multisample_count, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_property, VkImage *image);
Bool create_image_view(VulkanDevice *device, VkImage image, VkFormat format, VkImageAspectFlags aspect_mask, VkImageView *image_view);
Bool allocate_command_buffer(VulkanDevice *device, VkCommandBuffer *command_buffer);
Void free_command_buffer(VulkanDevice *device, VkCommandBuffer command_buffer);

Bool upload_buffer(VulkanDevice *device, VulkanBuffer *host_buffer, VulkanBuffer *device_buffer);
Bool upload_texture(VulkanDevice *device, VulkanBuffer *host_buffer, VkImage image, Int width, Int height);

Bool allocate_descriptor_set(VulkanDevice *device, VkDescriptorSetLayout *descriptor_set_layout,
                             VulkanBuffer *uniform_buffer, Int offset, Int range, VkDescriptorSet *descriptor_set);