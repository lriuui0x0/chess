#pragma once

#include "../lib/util.hpp"
#include "math.cpp"
#include "entity.cpp"

struct HemiLight
{
    Vec4 dir;
    Vec4 color;
    Vec4 opp_color;
};

struct SceneUniformData
{
    Mat4 view;
    Mat4 normal_view;
    Mat4 projection;
    HemiLight hemi_light;
};

struct EntityUniformData
{
    Mat4 world;
    Mat4 normal_world;
    Vec4 color_overlay;
};

VkSampleCountFlagBits get_maximum_multisample_count(VulkanDevice *device)
{
    VkSampleCountFlagBits multisample_count = VK_SAMPLE_COUNT_64_BIT;
    while (
        !(multisample_count & device->physical_device_properties.limits.framebufferColorSampleCounts) ||
        !(multisample_count & device->physical_device_properties.limits.framebufferDepthSampleCounts) ||
        !(multisample_count & device->physical_device_properties.limits.sampledImageColorSampleCounts) ||
        !(multisample_count & device->physical_device_properties.limits.sampledImageDepthSampleCounts))
    {
        multisample_count = (VkSampleCountFlagBits)((UInt)multisample_count >> 1);
    }
    return multisample_count;
}

Bool create_scene_pipeline(VulkanDevice *device, VulkanPipeline *pipeline)
{
    VkSampleCountFlagBits multisample_count = get_maximum_multisample_count(device);

    AttachmentInfo color_attachment;
    color_attachment.format = device->swapchain.format;
    color_attachment.multisample_count = multisample_count;
    color_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.working_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    Buffer<AttachmentInfo> color_attachments;
    color_attachments.count = 1;
    color_attachments.data = &color_attachment;

    AttachmentInfo depth_attachment;
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.multisample_count = multisample_count;
    depth_attachment.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.working_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    AttachmentInfo resolve_attachment;
    resolve_attachment.format = device->swapchain.format;
    resolve_attachment.multisample_count = VK_SAMPLE_COUNT_1_BIT;
    resolve_attachment.load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolve_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    resolve_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolve_attachment.working_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resolve_attachment.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if (!create_render_pass(device, &color_attachments, &depth_attachment, &resolve_attachment, &pipeline->render_pass))
    {
        return false;
    }

    Str vertex_shader_code;
    if (!read_file("scene.vert.spv", &vertex_shader_code))
    {
        return false;
    }

    Str fragment_shader_code;
    if (!read_file("scene.frag.spv", &fragment_shader_code))
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

    VertexAttributeInfo vertex_attribute_info[4];
    vertex_attribute_info[0].count = sizeof(Vec3);
    vertex_attribute_info[0].offset = offsetof(Vertex, pos);

    vertex_attribute_info[1].count = sizeof(Vec3);
    vertex_attribute_info[1].offset = offsetof(Vertex, normal);

    vertex_attribute_info[2].count = sizeof(Vec3);
    vertex_attribute_info[2].offset = offsetof(Vertex, color);

    vertex_attribute_info[3].count = sizeof(Vec2);
    vertex_attribute_info[3].offset = offsetof(Vertex, uv);

    Buffer<VertexAttributeInfo> vertex_attributes;
    vertex_attributes.count = 4;
    vertex_attributes.data = vertex_attribute_info;

    DescriptorBindingInfo scene_descriptor_binding;
    scene_descriptor_binding.stage = VK_SHADER_STAGE_VERTEX_BIT;
    scene_descriptor_binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    DescriptorBindingInfo piece_descriptor_binding;
    piece_descriptor_binding.stage = VK_SHADER_STAGE_VERTEX_BIT;
    piece_descriptor_binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    DescriptorBindingInfo lightmap_descriptor_binding;
    lightmap_descriptor_binding.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    lightmap_descriptor_binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    DescriptorSetInfo descriptor_set_info[3];
    descriptor_set_info[0].bindings.count = 1;
    descriptor_set_info[0].bindings.data = &scene_descriptor_binding;
    descriptor_set_info[1].bindings.count = 1;
    descriptor_set_info[1].bindings.data = &piece_descriptor_binding;
    descriptor_set_info[2].bindings.count = 1;
    descriptor_set_info[2].bindings.data = &lightmap_descriptor_binding;

    Buffer<DescriptorSetInfo> descriptor_sets;
    descriptor_sets.count = 3;
    descriptor_sets.data = descriptor_set_info;

    if (!create_pipeline(device, pipeline->render_pass, 0, &shaders, sizeof(Vertex), &vertex_attributes, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &descriptor_sets,
                         multisample_count, true, true, pipeline))
    {
        return false;
    }

    return true;
}

struct SceneFrame
{
    Array<VkFramebuffer> frame_buffers;
    VkCommandBuffer command_buffer;
    VkSemaphore image_aquired_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence frame_finished_fence;

    VkImage color_image;
    VkImage depth_image;
    VulkanBuffer vertex_buffer;
    VulkanBuffer index_buffer;
    VulkanBuffer uniform_buffer;
    VkDescriptorSet scene_descriptor_set;
    VkDescriptorSet board_descriptor_set;
    Array<VkImage> lightmap_images;
    Array<VkSampler> lightmap_samplers;
    Array<VkDescriptorSet> lightmap_descriptor_sets;
    Array<VkDescriptorSet> piece_descriptor_sets;
    VkDescriptorSet ghost_piece_descriptor_set;
};

Bool create_scene_frame(VulkanDevice *device, VulkanPipeline *pipeline, Board *board, Piece *pieces, Array<Image> *lightmaps, SceneFrame *frame,
                        VulkanBuffer *host_vertex_buffer, VulkanBuffer *host_index_buffer, VulkanBuffer *host_uniform_buffer)
{
    VkSampleCountFlagBits multisample_count = get_maximum_multisample_count(device);

    VkResult result_code;

    if (!allocate_command_buffer(device, &frame->command_buffer))
    {
        return false;
    }

    if (!create_semaphore(device, &frame->image_aquired_semaphore))
    {
        return false;
    }

    if (!create_semaphore(device, &frame->render_finished_semaphore))
    {
        return false;
    }

    if (!create_fence(device, true, &frame->frame_finished_fence))
    {
        return false;
    }

    if (!create_image(device, device->swapchain.width, device->swapchain.height,
                      device->swapchain.format, multisample_count, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &frame->color_image))
    {
        return false;
    }

    VkImageView color_image_view;
    if (!create_image_view(device, frame->color_image, device->swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, &color_image_view))
    {
        return false;
    }

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
        VkImageView image_view;
        if (!create_image_view(device, device->swapchain.images[image_i], device->swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, &image_view))
        {
            return false;
        }

        VkImageView attachments[3] = {color_image_view, depth_image_view, image_view};
        VkFramebufferCreateInfo frame_buffer_create_info = {};
        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = pipeline->render_pass;
        frame_buffer_create_info.attachmentCount = 3;
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

    frame->lightmap_images = create_array<VkImage>();
    frame->lightmap_samplers = create_array<VkSampler>();
    frame->lightmap_descriptor_sets = create_array<VkDescriptorSet>();
    for (Int lightmap_i = 0; lightmap_i < lightmaps->count; lightmap_i++)
    {
        VkFormat format = VK_FORMAT_R8_UNORM;
        Image *lightmap = &lightmaps->data[lightmap_i];
        VkImage *image = frame->lightmap_images.push();
        VkSampler *sampler = frame->lightmap_samplers.push();
        VkDescriptorSet *descriptor_set = frame->lightmap_descriptor_sets.push();
        if (!create_image(device, lightmap->width, lightmap->height,
                          format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image))
        {
            return false;
        }

        Int image_buffer_length = sizeof(lightmap->data[0]) * lightmap->width * lightmap->height;
        VulkanBuffer host_image_buffer;
        if (!create_buffer(device, image_buffer_length,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &host_image_buffer))
        {
            return false;
        }

        memcpy(host_image_buffer.data, lightmap->data, image_buffer_length);

        if (!upload_texture(device, &host_image_buffer, *image, lightmap->width, lightmap->height))
        {
            return false;
        }

        VkImageView image_view;
        if (!create_image_view(device, *image, format, VK_IMAGE_ASPECT_COLOR_BIT, &image_view))
        {
            return false;
        }

        if (!create_sampler(device, sampler))
        {
            return false;
        }

        if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[2], image_view, *sampler, descriptor_set))
        {
            return false;
        }
    }

    Int total_vertex_count = 0;
    board->mesh->vertex_offset = total_vertex_count;
    Int total_index_count = 0;
    board->mesh->index_offset = total_index_count;
    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        Piece *piece = &pieces[piece_i];
        piece->mesh->vertex_offset = total_vertex_count;
        total_vertex_count += piece->mesh->vertex_count;
        piece->mesh->index_offset = total_index_count;
        total_index_count += piece->mesh->index_count;
    }

    Int total_vertex_data_length = sizeof(Vertex) * total_vertex_count;
    Int total_index_data_length = sizeof(UInt32) * total_index_count;
    Int scene_uniform_data_length = align_up(sizeof(SceneUniformData), device->physical_device_properties.limits.minUniformBufferOffsetAlignment);
    Int entity_uniform_data_length = align_up(sizeof(EntityUniformData), device->physical_device_properties.limits.minUniformBufferOffsetAlignment);
    Int total_uniform_data_length = scene_uniform_data_length + entity_uniform_data_length * (PIECE_COUNT + 2);

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

    if (!create_buffer(device, total_index_data_length,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &frame->index_buffer))
    {
        return false;
    }

    if (!create_buffer(device, total_index_data_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       host_index_buffer))
    {
        return false;
    }

    if (!create_buffer(device, total_uniform_data_length,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &frame->uniform_buffer))
    {
        return false;
    }

    if (!create_buffer(device, total_uniform_data_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       host_uniform_buffer))
    {
        return false;
    }

    if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[0], &frame->uniform_buffer,
                                 0, scene_uniform_data_length, &frame->scene_descriptor_set))
    {
        return false;
    }

    if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[1], &frame->uniform_buffer,
                                 scene_uniform_data_length, entity_uniform_data_length, &frame->board_descriptor_set))
    {
        return false;
    }

    frame->piece_descriptor_sets = create_array<VkDescriptorSet>(PIECE_COUNT);
    frame->piece_descriptor_sets.count = PIECE_COUNT;
    for (Int piece_i = 0; piece_i < PIECE_COUNT; piece_i++)
    {
        if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[1], &frame->uniform_buffer,
                                     scene_uniform_data_length + (piece_i + 1) * entity_uniform_data_length, entity_uniform_data_length,
                                     &frame->piece_descriptor_sets[piece_i]))
        {
            return false;
        }
    }

    if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[1], &frame->uniform_buffer,
                                 scene_uniform_data_length + (PIECE_COUNT + 1) * entity_uniform_data_length, entity_uniform_data_length,
                                 &frame->ghost_piece_descriptor_set))
    {
        return false;
    }

    return true;
}

SceneUniformData *get_scene_uniform_data(VulkanDevice *device, VulkanBuffer *uniform_buffer)
{
    return (SceneUniformData *)uniform_buffer->data;
}

EntityUniformData *get_board_uniform_data(VulkanDevice *device, VulkanBuffer *uniform_buffer)
{
    Int offset = align_up(sizeof(SceneUniformData), device->physical_device_properties.limits.minUniformBufferOffsetAlignment);
    return (EntityUniformData *)(uniform_buffer->data + offset);
}

EntityUniformData *get_piece_uniform_data(VulkanDevice *device, VulkanBuffer *uniform_buffer, Int piece_index)
{
    Int offset = align_up(sizeof(SceneUniformData), device->physical_device_properties.limits.minUniformBufferOffsetAlignment);
    offset += align_up(sizeof(EntityUniformData), device->physical_device_properties.limits.minUniformBufferOffsetAlignment) * (piece_index + 1);
    return (EntityUniformData *)(uniform_buffer->data + offset);
}

EntityUniformData *get_ghost_piece_uniform_data(VulkanDevice *device, VulkanBuffer *uniform_buffer)
{
    Int offset = align_up(sizeof(SceneUniformData), device->physical_device_properties.limits.minUniformBufferOffsetAlignment);
    offset += align_up(sizeof(EntityUniformData), device->physical_device_properties.limits.minUniformBufferOffsetAlignment) * (PIECE_COUNT + 1);
    return (EntityUniformData *)(uniform_buffer->data + offset);
}

void calculate_scene_uniform_data(Camera *camera, Int window_width, Int window_height, SceneUniformData *uniform_data)
{
    Mat4 rotation = get_rotation_matrix(camera->rot);
    uniform_data->view = get_view_matrix(camera->pos, vec3(rotation.z), -vec3(rotation.y));
    uniform_data->normal_view = get_normal_view_matrix(camera->pos, vec3(rotation.z), -vec3(rotation.y));
    uniform_data->projection = get_perspective_matrix(degree_to_radian(30), (Real)window_width / (Real)window_height, 10, 10000);
}

void calculate_entity_uniform_data(Entity *entity, EntityUniformData *uniform_data)
{
    Mat4 translate = get_translate_matrix(entity->pos.x, entity->pos.y, entity->pos.z);
    Mat4 rotation = get_rotation_matrix(entity->rot);
    Mat4 scale = get_scale_matrix(entity->scale.x, entity->scale.y, entity->scale.z);
    uniform_data->world = translate * rotation * scale;
    uniform_data->normal_world = rotation * scale;
    uniform_data->color_overlay = entity->color_overlay;
}
