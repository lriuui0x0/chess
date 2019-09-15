#pragma once

#include "../lib/vulkan.hpp"
#include "math.cpp"
#include "entity.cpp"

struct DebugCollisionVertex
{
    Vec3 pos;
    Vec3 color;
};

Bool create_debug_collision_pipeline(VulkanDevice *device, VulkanPipeline *pipeline)
{
    AttachmentInfo color_attachment;
    color_attachment.format = device->swapchain.format;
    color_attachment.multisample_count = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.initial_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.working_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    Buffer<AttachmentInfo> color_attachments;
    color_attachments.count = 1;
    color_attachments.data = &color_attachment;

    if (!create_render_pass(device, &color_attachments, null, null, &pipeline->render_pass))
    {
        return false;
    }

    Str vertex_shader_code;
    if (!read_file("debug_collision.vert.spv", &vertex_shader_code))
    {
        return false;
    }

    Str fragment_shader_code;
    if (!read_file("debug_collision.frag.spv", &fragment_shader_code))
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
    vertex_attribute_info[0].count = sizeof(Vec3);
    vertex_attribute_info[0].offset = offsetof(DebugCollisionVertex, pos);

    vertex_attribute_info[1].count = sizeof(Vec3);
    vertex_attribute_info[1].offset = offsetof(DebugCollisionVertex, color);

    Buffer<VertexAttributeInfo> vertex_attributes;
    vertex_attributes.count = 2;
    vertex_attributes.data = vertex_attribute_info;

    DescriptorBindingInfo descriptor_binding;
    descriptor_binding.stage = VK_SHADER_STAGE_VERTEX_BIT;
    descriptor_binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    DescriptorSetInfo descriptor_set_info[2];
    descriptor_set_info[0].bindings.count = 1;
    descriptor_set_info[0].bindings.data = &descriptor_binding;
    descriptor_set_info[1].bindings.count = 1;
    descriptor_set_info[1].bindings.data = &descriptor_binding;

    Buffer<DescriptorSetInfo> descriptor_sets;
    descriptor_sets.count = 2;
    descriptor_sets.data = descriptor_set_info;

    if (!create_pipeline(device, pipeline->render_pass, 0, &shaders, sizeof(DebugCollisionVertex), &vertex_attributes, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, &descriptor_sets, VK_SAMPLE_COUNT_1_BIT, false, false, pipeline))
    {
        return false;
    }

    return true;
}

struct DebugCollisionFrame
{
    VulkanBuffer vertex_buffer;
    Array<VkFramebuffer> frame_buffers;
};

Bool create_debug_collision_frame(VulkanDevice *device, VulkanPipeline *pipeline, Array<Piece> *pieces, DebugCollisionFrame *frame, VulkanBuffer *host_vertex_buffer)
{
    VkResult result_code;

    frame->frame_buffers = create_array<VkFramebuffer>(device->swapchain.images.count);
    for (Int image_i = 0; image_i < device->swapchain.images.count; image_i++)
    {
        VkImageView image_view;
        if (!create_image_view(device, device->swapchain.images[image_i], device->swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, &image_view))
        {
            return false;
        }

        VkImageView attachments[1] = {image_view};
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

    Int total_vertex_data_length = 0;
    Int line_count = 12;
    for (Int piece_i = 0; piece_i < pieces->count; piece_i++)
    {
        total_vertex_data_length += sizeof(DebugCollisionVertex) * line_count * 2;
    }

    if (!create_buffer(device, total_vertex_data_length,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &frame->vertex_buffer))
    {
        return false;
    }

    if (!create_buffer(device, total_vertex_data_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       host_vertex_buffer))
    {
        return false;
    }

    return true;
}

Void write_collision_box_vertex_data(CollisionBox *collision_box, DebugCollisionVertex *vertex)
{
    {
        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;
    }

    {
        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;
    }

    {
        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z - collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;
    }

    {
        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x + collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y + collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;

        vertex->pos.x = collision_box->center.x - collision_box->radius.x;
        vertex->pos.y = collision_box->center.y - collision_box->radius.y;
        vertex->pos.z = collision_box->center.z + collision_box->radius.z;
        vertex->color = {1, 0, 0};
        vertex++;
    }
}
