#pragma once

#include "../lib/vulkan.hpp"
#include "math.cpp"
#include "scene.cpp"

struct BlurVertex
{
    Vec2 pos;
    Vec2 texture_coord;
};

Bool create_blur_pipeline(VulkanDevice *device, VulkanPipeline *pipeline)
{
    AttachmentInfo color_attachment;
    color_attachment.format = device->swapchain.format;
    color_attachment.multisample_count = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.working_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    Buffer<AttachmentInfo> color_attachments;
    color_attachments.count = 1;
    color_attachments.data = &color_attachment;

    if (!create_render_pass(device, &color_attachments, null, null, &pipeline->render_pass))
    {
        return false;
    }

    Str vertex_shader_code;
    if (!read_file("blur.vert.spv", &vertex_shader_code))
    {
        return false;
    }

    Str fragment_shader_code;
    if (!read_file("blur.frag.spv", &fragment_shader_code))
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

    VertexAttributeInfo vertex_attribute_info[2];
    vertex_attribute_info[0].count = sizeof(Vec2);
    vertex_attribute_info[0].offset = offsetof(BlurVertex, pos);

    vertex_attribute_info[1].count = sizeof(Vec2);
    vertex_attribute_info[1].offset = offsetof(BlurVertex, texture_coord);

    Buffer<VertexAttributeInfo> vertex_attributes;
    vertex_attributes.count = 2;
    vertex_attributes.data = vertex_attribute_info;

    DescriptorBindingInfo image_descriptor_binding;
    image_descriptor_binding.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    image_descriptor_binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    DescriptorBindingInfo uniform_descriptor_binding;
    uniform_descriptor_binding.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    uniform_descriptor_binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    DescriptorSetInfo descriptor_set_info[2];
    descriptor_set_info[0].bindings.count = 1;
    descriptor_set_info[0].bindings.data = &image_descriptor_binding;
    descriptor_set_info[1].bindings.count = 1;
    descriptor_set_info[1].bindings.data = &uniform_descriptor_binding;

    Buffer<DescriptorSetInfo> descriptor_sets;
    descriptor_sets.count = 2;
    descriptor_sets.data = descriptor_set_info;

    PushConstantInfo push_constant_info[1];
    push_constant_info[0].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    push_constant_info[0].size = 8;

    Buffer<PushConstantInfo> push_constants;
    push_constants.count = 1;
    push_constants.data = push_constant_info;

    if (!create_pipeline(device, pipeline->render_pass, 0, &shaders, sizeof(BlurVertex), &vertex_attributes, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &descriptor_sets, &push_constants,
                         VK_SAMPLE_COUNT_1_BIT, false, false, null, pipeline))
    {
        return false;
    }

    return true;
}

struct BlurUniformData
{
    Int blur_radius;
};

struct BlurFrame
{
    VkFramebuffer frame_buffer;
    VkFramebuffer frame_buffer2;
    VulkanBuffer vertex_buffer;
    VulkanBuffer uniform_buffer;
    VkSampler color_sampler;
    VkDescriptorSet color_descriptor_set;
    VkDescriptorSet uniform_descriptor_set;
    VkImage color_image2;
    VkSampler color_sampler2;
    VkDescriptorSet color_descriptor_set2;
};

Bool create_blur_frame(VulkanDevice *device, VulkanPipeline *pipeline, SceneFrame *scene_frame, BlurFrame *frame, VulkanBuffer *host_uniform_buffer)
{
    VkResult result_code;
    Int vertex_buffer_length = sizeof(BlurVertex) * 6;
    if (!create_buffer(device, vertex_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &frame->vertex_buffer))
    {
        return false;
    }

    VulkanBuffer host_vertex_buffer;
    if (!create_buffer(device, vertex_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &host_vertex_buffer))
    {
        return false;
    }

    Int uniform_buffer_length = sizeof(BlurUniformData);
    if (!create_buffer(device, uniform_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &frame->uniform_buffer))
    {
        return false;
    }

    if (!create_buffer(device, uniform_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       host_uniform_buffer))
    {
        return false;
    }

    BlurVertex *blur_vertex = (BlurVertex *)host_vertex_buffer.data;
    blur_vertex->pos = Vec2{-1.0, -1.0};
    blur_vertex->texture_coord = Vec2{0.0, 0.0};
    blur_vertex++;
    blur_vertex->pos = Vec2{1.0, -1.0};
    blur_vertex->texture_coord = Vec2{1.0, 0.0};
    blur_vertex++;
    blur_vertex->pos = Vec2{-1.0, 1.0};
    blur_vertex->texture_coord = Vec2{0.0, 1.0};
    blur_vertex++;
    blur_vertex->pos = Vec2{1.0, -1.0};
    blur_vertex->texture_coord = Vec2{1.0, 0.0};
    blur_vertex++;
    blur_vertex->pos = Vec2{1.0, 1.0};
    blur_vertex->texture_coord = Vec2{1.0, 1.0};
    blur_vertex++;
    blur_vertex->pos = Vec2{-1.0, 1.0};
    blur_vertex->texture_coord = Vec2{0.0, 1.0};
    blur_vertex++;
    if (!upload_buffer(device, &host_vertex_buffer, &frame->vertex_buffer))
    {
        return false;
    }

    if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[1], &frame->uniform_buffer,
                                 0, uniform_buffer_length, &frame->uniform_descriptor_set))
    {
        return false;
    }

    if (!create_sampler(device, &frame->color_sampler))
    {
        return false;
    }

    if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[0], scene_frame->color_image_view, frame->color_sampler, &frame->color_descriptor_set))
    {
        return false;
    }

    if (!create_image(device, device->swapchain.width, device->swapchain.height,
                      device->swapchain.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &frame->color_image2))
    {
        return false;
    }

    VkImageView color_image_view2;
    if (!create_image_view(device, frame->color_image2, device->swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, &color_image_view2))
    {
        return false;
    }

    if (!create_sampler(device, &frame->color_sampler2))
    {
        return false;
    }

    if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[0], color_image_view2, frame->color_sampler2, &frame->color_descriptor_set2))
    {
        return false;
    }

    VkImageView attachments[1] = {color_image_view2};
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

    attachments[0] = scene_frame->color_image_view;

    result_code = vkCreateFramebuffer(device->handle, &frame_buffer_create_info, null, &frame->frame_buffer2);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    return true;
}
