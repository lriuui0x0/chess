#pragma once

#include "../lib/vulkan.hpp"
#include "../lib/util.hpp"
#include "math.cpp"

Bool create_shadow_pipeline(VulkanDevice *device, VulkanPipeline *pipeline)
{
    AttachmentInfo depth_attachment;
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.multisample_count = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.working_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (!create_render_pass(device, null, &depth_attachment, null, &pipeline->render_pass))
    {
        return false;
    }

    Str vertex_shader_code;
    if (!read_file("shadow.vert.spv", &vertex_shader_code))
    {
        return false;
    }

    Str fragment_shader_code;
    if (!read_file("shadow.frag.spv", &fragment_shader_code))
    {
        return false;
    }

    ShaderInfo shader_info[2];
    shader_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_info[0].code = vertex_shader_code;

    shader_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_info[1].code = fragment_shader_code;

    Buffer<ShaderInfo> shaders;
    shaders.count = 2;
    shaders.data = shader_info;

    VertexAttributeInfo vertex_attribute_info[1];
    vertex_attribute_info[0].count = sizeof(Vec3);
    vertex_attribute_info[0].offset = offsetof(Vertex, pos);

    Buffer<VertexAttributeInfo> vertex_attributes;
    vertex_attributes.count = 1;
    vertex_attributes.data = vertex_attribute_info;

    DescriptorBindingInfo shadow_scene_descriptor_binding;
    shadow_scene_descriptor_binding.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shadow_scene_descriptor_binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    DescriptorBindingInfo piece_descriptor_binding;
    piece_descriptor_binding.stage = VK_SHADER_STAGE_VERTEX_BIT;
    piece_descriptor_binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    DescriptorSetInfo descriptor_set_info[2];
    descriptor_set_info[0].bindings.count = 1;
    descriptor_set_info[0].bindings.data = &shadow_scene_descriptor_binding;
    descriptor_set_info[1].bindings.count = 1;
    descriptor_set_info[1].bindings.data = &piece_descriptor_binding;

    Buffer<DescriptorSetInfo> descriptor_sets;
    descriptor_sets.count = 2;
    descriptor_sets.data = descriptor_set_info;

    DepthBias depth_bias;
    depth_bias.const_bias = 1.5;
    depth_bias.slope_bias = 1.5;

    if (!create_pipeline(device, pipeline->render_pass, 0, &shaders, sizeof(Vertex), &vertex_attributes, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &descriptor_sets, null,
                         VK_SAMPLE_COUNT_1_BIT, true, false, &depth_bias, pipeline))
    {
        return false;
    }

    return true;
}

struct ShadowFrame
{
    VkImage depth_image;
    VkFramebuffer frame_buffer;
};

Bool create_shadow_frame(VulkanDevice *device, VulkanPipeline *pipeline, ShadowFrame *frame)
{
    VkResult result_code;
    if (!create_image(device, device->swapchain.width, device->swapchain.height,
                      VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &frame->depth_image))
    {
        return false;
    }

    VkImageView depth_image_view;
    if (!create_image_view(device, frame->depth_image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_view))
    {
        return false;
    }

    VkImageView attachments[1] = {depth_image_view};
    VkFramebufferCreateInfo frame_buffer_create_info = {};
    frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frame_buffer_create_info.renderPass = pipeline->render_pass;
    frame_buffer_create_info.attachmentCount = 1;
    frame_buffer_create_info.pAttachments = attachments;
    frame_buffer_create_info.width = device->swapchain.width;
    frame_buffer_create_info.height = device->swapchain.height;
    frame_buffer_create_info.layers = 1;

    result_code = vkCreateFramebuffer(device->handle, &frame_buffer_create_info, null, &frame->frame_buffer);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}
