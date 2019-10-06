#pragma once

#include "../lib/vulkan.hpp"
#include "math.cpp"
#include "entity.cpp"

struct DebugMoveVertex
{
    Vec3 pos;
    Vec3 color;
};

Bool create_debug_move_pipeline(VulkanDevice *device, VulkanPipeline *pipeline)
{
    VkSampleCountFlagBits multisample_count = get_maximum_multisample_count(device);

    AttachmentInfo color_attachment;
    color_attachment.format = device->swapchain.format;
    color_attachment.multisample_count = multisample_count;
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
    if (!read_file("debug_move.vert.spv", &vertex_shader_code))
    {
        return false;
    }

    Str fragment_shader_code;
    if (!read_file("debug_move.frag.spv", &fragment_shader_code))
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
    vertex_attribute_info[0].offset = offsetof(DebugMoveVertex, pos);

    vertex_attribute_info[1].count = sizeof(Vec3);
    vertex_attribute_info[1].offset = offsetof(DebugMoveVertex, color);

    Buffer<VertexAttributeInfo> vertex_attributes;
    vertex_attributes.count = 2;
    vertex_attributes.data = vertex_attribute_info;

    DescriptorBindingInfo scene_descriptor_binding;
    scene_descriptor_binding.stage = VK_SHADER_STAGE_VERTEX_BIT;
    scene_descriptor_binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    DescriptorSetInfo descriptor_set_info[1];
    descriptor_set_info[0].bindings.count = 1;
    descriptor_set_info[0].bindings.data = &scene_descriptor_binding;

    Buffer<DescriptorSetInfo> descriptor_sets;
    descriptor_sets.count = 1;
    descriptor_sets.data = descriptor_set_info;

    if (!create_pipeline(device, pipeline->render_pass, 0, &shaders, sizeof(DebugMoveVertex), &vertex_attributes, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &descriptor_sets,
                         multisample_count, false, true, null, pipeline))
    {
        return false;
    }

    return true;
}

struct DebugMoveFrame
{
    VulkanBuffer vertex_buffer;
    VkFramebuffer frame_buffer;
};

Bool create_debug_move_frame(VulkanDevice *device, VulkanPipeline *pipeline, SceneFrame *scene_frame, DebugMoveFrame *frame, VulkanBuffer *host_vertex_buffer)
{
    VkResult result_code;

    VkImageView attachments[1] = {scene_frame->color_image_view};
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

    Int total_vertex_data_length = sizeof(DebugMoveVertex) * 6 * 64;
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

struct DebugMoveDrawState
{
    VulkanBuffer *vertex_buffer;
    Int count;
};

Void debug_move_draw(DebugMoveDrawState *draw_state, Int square, Vec3 color)
{
    Vec3 square_pos = get_square_pos(square);
    DebugMoveVertex *vertex = (DebugMoveVertex *)draw_state->vertex_buffer->data + draw_state->count++ * 6;

    vertex->pos = Vec3{square_pos.x - SQUARE_SIZE / 2, square_pos.y, square_pos.z - SQUARE_SIZE / 2};
    vertex->color = color;
    vertex++;

    vertex->pos = Vec3{square_pos.x + SQUARE_SIZE / 2, square_pos.y, square_pos.z + SQUARE_SIZE / 2};
    vertex->color = color;
    vertex++;

    vertex->pos = Vec3{square_pos.x - SQUARE_SIZE / 2, square_pos.y, square_pos.z + SQUARE_SIZE / 2};
    vertex->color = color;
    vertex++;

    vertex->pos = Vec3{square_pos.x + SQUARE_SIZE / 2, square_pos.y, square_pos.z + SQUARE_SIZE / 2};
    vertex->color = color;
    vertex++;

    vertex->pos = Vec3{square_pos.x - SQUARE_SIZE / 2, square_pos.y, square_pos.z - SQUARE_SIZE / 2};
    vertex->color = color;
    vertex++;

    vertex->pos = Vec3{square_pos.x + SQUARE_SIZE / 2, square_pos.y, square_pos.z - SQUARE_SIZE / 2};
    vertex->color = color;
    vertex++;
}
