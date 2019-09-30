#pragma once

#include "../lib/vulkan.hpp"
#include "../lib/util.hpp"
#include "math.cpp"

struct ShadowVertex
{
    Vec3 pos;
};

struct ShadowUniformData
{
    Mat4 view;
    Mat4 projection;
};

Bool create_shadow_pipeline(VulkanDevice *device, VulkanPipeline *pipeline)
{
    VkSampleCountFlagBits multisample_count = get_maximum_multisample_count(device);

    AttachmentInfo depth_attachment;
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.multisample_count = multisample_count;
    depth_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.working_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

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
    vertex_attribute_info[0].offset = offsetof(ShadowVertex, pos);

    Buffer<VertexAttributeInfo> vertex_attributes;
    vertex_attributes.count = 1;
    vertex_attributes.data = vertex_attribute_info;

    DescriptorBindingInfo shadow_scene_descriptor_binding;
    shadow_scene_descriptor_binding.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shadow_scene_descriptor_binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    DescriptorBindingInfo piece_descriptor_binding;
    piece_descriptor_binding.stage = VK_SHADER_STAGE_VERTEX_BIT;
    piece_descriptor_binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    DescriptorSetInfo descriptor_set_info[3];
    descriptor_set_info[0].bindings.count = 1;
    descriptor_set_info[0].bindings.data = &shadow_scene_descriptor_binding;
    descriptor_set_info[1].bindings.count = 1;
    descriptor_set_info[1].bindings.data = &piece_descriptor_binding;

    Buffer<DescriptorSetInfo> descriptor_sets;
    descriptor_sets.count = 2;
    descriptor_sets.data = descriptor_set_info;

    if (!create_pipeline(device, pipeline->render_pass, 0, &shaders, sizeof(ShadowVertex), &vertex_attributes, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &descriptor_sets,
                         multisample_count, true, false, pipeline))
    {
        return false;
    }

    return true;
}

struct ShadowFrame
{
    Array<VkFramebuffer> frame_buffers;
    VkImage depth_image;
    VulkanBuffer uniform_buffer;
    VkDescriptorSet shadow_descriptor_set;
};

Bool create_shadow_frame(VulkanDevice *device, VulkanPipeline *pipeline, ShadowFrame *frame, VulkanBuffer *host_uniform_buffer)
{
    VkSampleCountFlagBits multisample_count = get_maximum_multisample_count(device);

    VkResult result_code;

    if (!create_image(device, device->swapchain.width, device->swapchain.height,
                      VK_FORMAT_D32_SFLOAT, multisample_count, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &frame->depth_image))
    {
        return false;
    }

    VkImageView depth_image_view;
    if (!create_image_view(device, frame->depth_image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_view))
    {
        return false;
    }

    frame->frame_buffers = create_array<VkFramebuffer>(device->swapchain.images.count);
    for (Int image_i = 0; image_i < device->swapchain.images.count; image_i++)
    {
        VkImageView attachments[1] = {depth_image_view};
        VkFramebufferCreateInfo frame_buffer_create_info = {};
        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = pipeline->render_pass;
        frame_buffer_create_info.attachmentCount = 1;
        frame_buffer_create_info.pAttachments = attachments;
        frame_buffer_create_info.width = device->swapchain.width;
        frame_buffer_create_info.height = device->swapchain.height;
        frame_buffer_create_info.layers = 1;

        VkFramebuffer *frame_buffer = frame->frame_buffers.push();
        result_code = vkCreateFramebuffer(device->handle, &frame_buffer_create_info, null, frame_buffer);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }
    }

    Int uniform_data_length = align_up(sizeof(ShadowUniformData), device->physical_device_properties.limits.minUniformBufferOffsetAlignment);
    if (!create_buffer(device, uniform_data_length,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &frame->uniform_buffer))
    {
        return false;
    }

    if (!create_buffer(device, uniform_data_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       host_uniform_buffer))
    {
        return false;
    }

    if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[0], &frame->uniform_buffer,
                                 0, uniform_data_length, &frame->shadow_descriptor_set))
    {
        return false;
    }

    return true;
}

Void calculate_shadow_uniform_data(Vec3 camera_pos, Mat3 camera_rot, ShadowUniformData *uniform_data)
{
    uniform_data->view = get_view_matrix(camera_pos, camera_rot.z, -camera_rot.y);
    // NOTE: Keep orthographic projection x:y ratio 4:3 to match window size
    uniform_data->projection = get_orthographic_matrix(-650, 950, -200, 1000, 400, 1100);
}
