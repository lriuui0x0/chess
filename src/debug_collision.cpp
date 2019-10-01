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

    if (!create_pipeline(device, pipeline->render_pass, 0, &shaders, sizeof(DebugCollisionVertex), &vertex_attributes, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, &descriptor_sets,
                         VK_SAMPLE_COUNT_1_BIT, false, false, null, pipeline))
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

#define COLLISION_BOX_VERTEX_COUNT (6 * 4 * 2)

Bool create_debug_collision_frame(VulkanDevice *device, VulkanPipeline *pipeline, Piece *pieces, DebugCollisionFrame *frame, VulkanBuffer *host_vertex_buffer)
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
    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        Piece *piece = &pieces[piece_i];
        total_vertex_data_length += sizeof(DebugCollisionVertex) * (COLLISION_BOX_VERTEX_COUNT + piece->mesh->collision_hull_vertex_count * 2);
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

DebugCollisionVertex *write_collision_box_face_data(Vec3 a, Vec3 b, Vec3 c, Vec3 d, DebugCollisionVertex *vertex)
{
    Vec3 color = {1, 0, 0};
    vertex->pos = a;
    vertex->color = color;
    vertex++;

    vertex->pos = b;
    vertex->color = color;
    vertex++;

    vertex->pos = b;
    vertex->color = color;
    vertex++;

    vertex->pos = c;
    vertex->color = color;
    vertex++;

    vertex->pos = c;
    vertex->color = color;
    vertex++;

    vertex->pos = d;
    vertex->color = color;
    vertex++;

    vertex->pos = d;
    vertex->color = color;
    vertex++;

    vertex->pos = a;
    vertex->color = color;
    vertex++;

    return vertex;
}

DebugCollisionVertex *write_collision_data(Mesh *mesh, DebugCollisionVertex *vertex)
{
    CollisionBox *collision_box = &mesh->collision_box;
    Real max_x = MAX(collision_box->center.x + collision_box->radius.x, collision_box->center.x - collision_box->radius.x);
    Real min_x = MIN(collision_box->center.x + collision_box->radius.x, collision_box->center.x - collision_box->radius.x);
    Real max_y = MAX(collision_box->center.y + collision_box->radius.y, collision_box->center.y - collision_box->radius.y);
    Real min_y = MIN(collision_box->center.y + collision_box->radius.y, collision_box->center.y - collision_box->radius.y);
    Real max_z = MAX(collision_box->center.z + collision_box->radius.z, collision_box->center.z - collision_box->radius.z);
    Real min_z = MIN(collision_box->center.z + collision_box->radius.z, collision_box->center.z - collision_box->radius.z);

    // NOTE: Top face
    vertex = write_collision_box_face_data(Vec3{max_x, min_y, min_z}, Vec3{max_x, min_y, max_z}, Vec3{min_x, min_y, max_z}, Vec3{min_x, min_y, min_z}, vertex);
    // NOTE: Bottom face
    vertex = write_collision_box_face_data(Vec3{max_x, max_y, min_z}, Vec3{max_x, max_y, max_z}, Vec3{min_x, max_y, max_z}, Vec3{min_x, max_y, min_z}, vertex);
    // NOTE: Front face
    vertex = write_collision_box_face_data(Vec3{max_x, max_y, min_z}, Vec3{max_x, min_y, min_z}, Vec3{min_x, min_y, min_z}, Vec3{min_x, max_y, min_z}, vertex);
    // NOTE: Back face
    vertex = write_collision_box_face_data(Vec3{max_x, max_y, max_z}, Vec3{max_x, min_y, max_z}, Vec3{min_x, min_y, max_z}, Vec3{min_x, max_y, max_z}, vertex);
    // NOTE: Right face
    vertex = write_collision_box_face_data(Vec3{max_x, max_y, max_z}, Vec3{max_x, min_y, max_z}, Vec3{max_x, min_y, min_z}, Vec3{max_x, max_y, min_z}, vertex);
    // NOTE: Left face
    vertex = write_collision_box_face_data(Vec3{min_x, max_y, max_z}, Vec3{min_x, min_y, max_z}, Vec3{min_x, min_y, min_z}, Vec3{min_x, max_y, min_z}, vertex);

    for (Int vertex_i = 0; vertex_i < mesh->collision_hull_vertex_count; vertex_i += 3)
    {
        Vec3 pos[3];
        for (Int i = 0; i < 3; i++)
        {
            pos[i] = mesh->collision_hull_vertex_data[vertex_i + i];
        }

        Vec3 color = {0, 0, 1};
        vertex->pos = pos[0];
        vertex->color = color;
        vertex++;

        vertex->pos = pos[1];
        vertex->color = color;
        vertex++;

        vertex->pos = pos[1];
        vertex->color = color;
        vertex++;

        vertex->pos = pos[2];
        vertex->color = color;
        vertex++;

        vertex->pos = pos[2];
        vertex->color = color;
        vertex++;

        vertex->pos = pos[0];
        vertex->color = color;
        vertex++;
    }
    return vertex;
}
