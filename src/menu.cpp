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
    Real font_type;
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

    VertexAttributeInfo vertex_attribute_info[4];
    vertex_attribute_info[0].count = sizeof(Vec2);
    vertex_attribute_info[0].offset = offsetof(MenuVertex, pos);

    vertex_attribute_info[1].count = sizeof(Vec2);
    vertex_attribute_info[1].offset = offsetof(MenuVertex, texture_coord);

    vertex_attribute_info[2].count = sizeof(Vec4);
    vertex_attribute_info[2].offset = offsetof(MenuVertex, color);

    vertex_attribute_info[3].count = sizeof(Int);
    vertex_attribute_info[3].offset = offsetof(MenuVertex, font_type);

    Buffer<VertexAttributeInfo> vertex_attributes;
    vertex_attributes.count = 4;
    vertex_attributes.data = vertex_attribute_info;

    DescriptorBindingInfo descriptor_binding;
    descriptor_binding.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_binding.count = MENU_FONT_COUNT;

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

    VkImage font_textures[MENU_FONT_COUNT];
    VkSampler font_texture_samplers[MENU_FONT_COUNT];
    VkDescriptorSet font_texture_descriptor_set;
};

#define MENU_VERTEX_COUNT (109 * 6)

Bool create_menu_frame(VulkanDevice *device, VulkanPipeline *pipeline, SceneFrame *scene_frame, MenuFrame *frame, Font *menu_fonts, VulkanBuffer *host_vertex_buffer)
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

    VkImageView *font_texture_views = (VkImageView *)ALLOCA(sizeof(VkImageView) * MENU_FONT_COUNT);
    for (Int font_i = 0; font_i < MENU_FONT_COUNT; font_i++)
    {
        Font *font = &menu_fonts[font_i];

        if (!create_image(device, font->width, font->height,
                          device->swapchain.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &frame->font_textures[font_i]))
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

        if (!upload_texture(device, &host_image_buffer, frame->font_textures[font_i], font->width, font->height))
        {
            return false;
        }

        if (!create_image_view(device, frame->font_textures[font_i], device->swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, &font_texture_views[font_i]))
        {
            return false;
        }

        if (!create_sampler(device, &frame->font_texture_samplers[font_i]))
        {
            return false;
        }
    }

    if (!allocate_descriptor_set(device, pipeline->descriptor_set_layouts[0], MENU_FONT_COUNT, font_texture_views, frame->font_texture_samplers,
                                 &frame->font_texture_descriptor_set))
    {
        return false;
    }

    return true;
}

Void draw_char(CharTextureInfo *texture_info, MenuVertex *menu_vertex, Vec2 pos, Real alpha, Int font_type)
{
    Vec2 char_pos = {pos.x + texture_info->left_bearing, pos.y};
    menu_vertex[0].pos = char_pos;
    menu_vertex[0].texture_coord = {texture_info->uv_x_min, 0};
    menu_vertex[0].color = Vec4{0, 0, 0, alpha};
    menu_vertex[0].font_type = font_type;

    menu_vertex[1].pos = {char_pos.x, char_pos.y + texture_info->height};
    menu_vertex[1].texture_coord = {texture_info->uv_x_min, 1};
    menu_vertex[1].color = Vec4{0, 0, 0, alpha};
    menu_vertex[1].font_type = font_type;

    menu_vertex[2].pos = {char_pos.x + texture_info->width, char_pos.y};
    menu_vertex[2].texture_coord = {texture_info->uv_x_max, 0};
    menu_vertex[2].color = Vec4{0, 0, 0, alpha};
    menu_vertex[2].font_type = font_type;

    menu_vertex[3].pos = {char_pos.x + texture_info->width, char_pos.y + texture_info->height};
    menu_vertex[3].texture_coord = {texture_info->uv_x_max, 1};
    menu_vertex[3].color = Vec4{0, 0, 0, alpha};
    menu_vertex[3].font_type = font_type;

    menu_vertex[4].pos = {char_pos.x + texture_info->width, char_pos.y};
    menu_vertex[4].texture_coord = {texture_info->uv_x_max, 0};
    menu_vertex[4].color = Vec4{0, 0, 0, alpha};
    menu_vertex[4].font_type = font_type;

    menu_vertex[5].pos = {char_pos.x, char_pos.y + texture_info->height};
    menu_vertex[5].texture_coord = {texture_info->uv_x_min, 1};
    menu_vertex[5].color = Vec4{0, 0, 0, alpha};
    menu_vertex[5].font_type = font_type;
}

Vec2 draw_string(Font *font, MenuVertex *menu_vertex, Str string, Vec2 pos, Real alpha, Real font_type, Int window_width, Int window_height)
{
    for (Int8 char_i = 0; char_i < string.count; char_i++)
    {
        CharTextureInfo texture_info = get_char_texture_info(font, string[char_i], window_width, window_height);
        draw_char(&texture_info, menu_vertex, pos, alpha, font_type);
        pos.x += texture_info.advance;
        menu_vertex += 6;
    }
    pos.y += get_line_texture_info(font, window_width, window_height);
    return pos;
}

// MenuVertex *draw_selector(MenuVertex *menu_vertex, Vec2 pos, Real alpha)
// {
//     Real width = 0.015;
//     Real height = 0.015;
//     Real gap = 0.06;

//     menu_vertex[0].pos = {pos.x - width, pos.y - gap - height};
//     menu_vertex[0].color = Vec4{1, 1, 1, alpha};
//     menu_vertex[0].font_type = -1;

//     menu_vertex[1].pos = {pos.x + width, pos.y - gap - height};
//     menu_vertex[1].color = Vec4{1, 1, 1, alpha};
//     menu_vertex[1].font_type = -1;

//     menu_vertex[2].pos = {pos.x, pos.y - gap};
//     menu_vertex[2].color = Vec4{1, 1, 1, alpha};
//     menu_vertex[2].font_type = -1;

//     menu_vertex[3].pos = {pos.x - width, pos.y + gap + height};
//     menu_vertex[3].color = Vec4{1, 1, 1, alpha};
//     menu_vertex[3].font_type = -1;

//     menu_vertex[4].pos = {pos.x + width, pos.y + gap + height};
//     menu_vertex[4].color = Vec4{1, 1, 1, alpha};
//     menu_vertex[4].font_type = -1;

//     menu_vertex[5].pos = {pos.x, pos.y + gap};
//     menu_vertex[5].color = Vec4{1, 1, 1, alpha};
//     menu_vertex[5].font_type = -1;

//     menu_vertex += 6;
//     return menu_vertex;
// }

Void draw_underline(MenuVertex *menu_vertex, Vec2 pos, Real width, Real height, Real alpha, Real alpha_right)
{
    menu_vertex[0].pos = pos;
    menu_vertex[0].color = Vec4{1, 1, 1, alpha};
    menu_vertex[0].font_type = -1;

    menu_vertex[1].pos = {pos.x, pos.y + height};
    menu_vertex[1].color = Vec4{1, 1, 1, alpha};
    menu_vertex[1].font_type = -1;

    menu_vertex[2].pos = {pos.x + width, pos.y};
    menu_vertex[2].color = Vec4{1, 1, 1, alpha_right};
    menu_vertex[2].font_type = -1;

    menu_vertex[3].pos = {pos.x + width, pos.y + height};
    menu_vertex[3].color = Vec4{1, 1, 1, alpha_right};
    menu_vertex[3].font_type = -1;

    menu_vertex[4].pos = {pos.x + width, pos.y};
    menu_vertex[4].color = Vec4{1, 1, 1, alpha_right};
    menu_vertex[4].font_type = -1;

    menu_vertex[5].pos = {pos.x, pos.y + height};
    menu_vertex[5].color = Vec4{1, 1, 1, alpha};
    menu_vertex[5].font_type = -1;
}

struct MenuLayout
{
    Vec2 player_pos[GameSide::count];
    Vec2 player_end_pos[GameSide::count];
};

MenuLayout draw_menu(Font *menu_fonts, Real alpha, Int window_width, Int window_height, VulkanBuffer *vertex_buffer)
{
    MenuLayout menu_layout;
    MenuVertex *menu_vertex = (MenuVertex *)vertex_buffer->data;
    Int total_character_count = 0;

    Str player = str("Player");
    Vec2 player_pos = Vec2{-0.8, -0.8};
    draw_string(&menu_fonts[MENU_LARGE_FONT], menu_vertex, player, player_pos, alpha, MENU_LARGE_FONT, window_width, window_height);
    menu_vertex += 6 * player.count;
    total_character_count += player.count;

    Str white = str("white");
    Vec2 white_pos = Vec2{0.0, -0.78};
    Vec2 white_end_pos = draw_string(&menu_fonts[MENU_MEDIUM_FONT], menu_vertex, white, white_pos, alpha, MENU_MEDIUM_FONT, window_width, window_height);
    menu_vertex += 6 * white.count;
    total_character_count += white.count;

    menu_layout.player_pos[GameSide::white] = white_pos;
    menu_layout.player_end_pos[GameSide::white] = white_end_pos;

    draw_underline(menu_vertex, Vec2{white_pos.x, white_end_pos.y}, white_end_pos.x - white_pos.x, 0.005, alpha, alpha);
    menu_vertex += 6;
    total_character_count++;

    Str black = str("black");
    Vec2 black_pos = Vec2{0.35, -0.78};
    Vec2 black_end_pos = draw_string(&menu_fonts[MENU_MEDIUM_FONT], menu_vertex, black, black_pos, alpha, MENU_MEDIUM_FONT, window_width, window_height);
    menu_vertex += 6 * black.count;
    total_character_count += black.count;

    menu_layout.player_pos[GameSide::black] = black_pos;
    menu_layout.player_end_pos[GameSide::black] = black_end_pos;

    Vec2 rule_pos[7] = {
        {-0.8, 0.5},
        {0.0, 0.5},
        {-0.8, 0.6},
        {0.0, 0.6},
        {-0.8, 0.7},
        {0.0, 0.7},
        {-0.8, 0.8},
    };
    Str rules[7] = {
        str("Select - L click"),
        str("Undo - Z"),
        str("Move - L click"),
        str("Redo - R"),
        str("Deselect - R click"),
        str("Menu - Esc"),
        str("Promote - R click"),
    };
    for (Int rule_i = 0; rule_i < 7; rule_i++)
    {
        Str rule = rules[rule_i];
        draw_string(&menu_fonts[MENU_SMALL_FONT], menu_vertex, rule, rule_pos[rule_i], alpha, MENU_SMALL_FONT, window_width, window_height);
        menu_vertex += 6 * rule.count;
        total_character_count += rule.count;
    }

    draw_underline(menu_vertex, Vec2{-0.8, 0.45}, 1.5, 0.0025, alpha, 0.1);
    menu_vertex += 6;
    total_character_count++;

    ASSERT(total_character_count * 6 == MENU_VERTEX_COUNT);

    return menu_layout;
}
