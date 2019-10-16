#pragma once

#include "../lib/vulkan.hpp"
#include "math.cpp"
#include "asset.cpp"
#include "scene.cpp"

struct MenuVertex
{
    Vec2 pos;
    Vec2 texture_coord;
    Vec4 color;
};

Bool create_menu_pipeline(VulkanDevice *device, VulkanPipeline *pipeline)
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
    if (!read_file("menu.vert.spv", &vertex_shader_code))
    {
        return false;
    }

    Str fragment_shader_code;
    if (!read_file("menu.frag.spv", &fragment_shader_code))
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

    VertexAttributeInfo vertex_attribute_info[3];
    vertex_attribute_info[0].count = sizeof(Vec2);
    vertex_attribute_info[0].offset = offsetof(MenuVertex, pos);

    vertex_attribute_info[1].count = sizeof(Vec2);
    vertex_attribute_info[1].offset = offsetof(MenuVertex, texture_coord);

    vertex_attribute_info[2].count = sizeof(Vec4);
    vertex_attribute_info[2].offset = offsetof(MenuVertex, color);

    Buffer<VertexAttributeInfo> vertex_attributes;
    vertex_attributes.count = 2;
    vertex_attributes.data = vertex_attribute_info;

    DescriptorBindingInfo descriptor_binding;
    descriptor_binding.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    DescriptorSetInfo descriptor_set_info[1];
    descriptor_set_info[0].bindings.count = 1;
    descriptor_set_info[0].bindings.data = &descriptor_binding;

    Buffer<DescriptorSetInfo> descriptor_sets;
    descriptor_sets.count = 1;
    descriptor_sets.data = descriptor_set_info;

    if (!create_pipeline(device, pipeline->render_pass, 0, &shaders, sizeof(MenuVertex), &vertex_attributes, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &descriptor_sets, null,
                         VK_SAMPLE_COUNT_1_BIT, false, true, null, pipeline))
    {
        return false;
    }

    return true;
}

struct MenuFrame
{
    VulkanBuffer vertex_buffer;
    VkFramebuffer frame_buffer;
    VkImage font_texture;
    VkSampler font_texture_sampler;
    VkDescriptorSet font_texture_descriptor_set;
};

#define MENU_VERTEX_COUNT (30)

Bool create_menu_frame(VulkanDevice *device, VulkanPipeline *pipeline, Font *font, SceneFrame *scene_frame, MenuFrame *frame, VulkanBuffer *host_vertex_buffer)
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

    Int vertex_buffer_length = sizeof(MenuVertex) * MENU_VERTEX_COUNT;
    if (!create_buffer(device, vertex_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &frame->vertex_buffer))
    {
        return false;
    }

    if (!create_buffer(device, vertex_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       host_vertex_buffer))
    {
        return false;
    }

    if (!create_image(device, font->width, font->height,
                      device->swapchain.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &frame->font_texture))
    {
        return false;
    }

    Int image_buffer_length = sizeof(font->data[0]) * font->width * font->height;
    VulkanBuffer host_image_buffer;
    if (!create_buffer(device, image_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &host_image_buffer))
    {
        return false;
    }

    memcpy(host_image_buffer.data, font->data, image_buffer_length);

    if (!upload_texture(device, &host_image_buffer, frame->font_texture, font->width, font->height))
    {
        return false;
    }

    VkImageView font_texture_view;
    if (!create_image_view(device, frame->font_texture, device->swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, &font_texture_view))
    {
        return false;
    }

    if (!create_sampler(device, &frame->font_texture_sampler))
    {
        return false;
    }

    if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[0], font_texture_view, frame->font_texture_sampler, &frame->font_texture_descriptor_set))
    {
        return false;
    }

    return true;
}

Void draw_menu(Font *menu_font, Int window_width, Int window_height, VulkanBuffer *vertex_buffer)
{
    MenuVertex *menu_vertex = (MenuVertex *)vertex_buffer->data;

    Str title = str("Chess");
    Vec2 pos = {-0.8, -0.8};
    for (Int8 char_i = 0; char_i < title.count; char_i++)
    {
        Int8 character = title[char_i];
        CharTextureInfo texture_info = get_char_texture_info(menu_font, character, window_width, window_height);

        Vec2 char_pos = {pos.x + texture_info.left_bearing, pos.y};
        menu_vertex[0].pos = char_pos;
        menu_vertex[0].texture_coord = {texture_info.uv_x_min, 0};

        menu_vertex[1].pos = {char_pos.x, char_pos.y + texture_info.height};
        menu_vertex[1].texture_coord = {texture_info.uv_x_min, 1};

        menu_vertex[2].pos = {char_pos.x + texture_info.width, char_pos.y};
        menu_vertex[2].texture_coord = {texture_info.uv_x_max, 0};

        menu_vertex[3].pos = {char_pos.x + texture_info.width, char_pos.y + texture_info.height};
        menu_vertex[3].texture_coord = {texture_info.uv_x_max, 1};

        menu_vertex[4].pos = {char_pos.x + texture_info.width, char_pos.y};
        menu_vertex[4].texture_coord = {texture_info.uv_x_max, 0};

        menu_vertex[5].pos = {char_pos.x, char_pos.y + texture_info.height};
        menu_vertex[5].texture_coord = {texture_info.uv_x_min, 1};

        menu_vertex += 6;
        pos.x += texture_info.advance;
    }
}
