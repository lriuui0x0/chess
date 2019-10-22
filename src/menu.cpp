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

#define MAX_MENU_VERTEX_COUNT (300 * 6)

Bool create_menu_frame(VulkanDevice *device, VulkanPipeline *pipeline, SceneFrame *scene_frame, MenuFrame *frame, BitmapFont *menu_fonts, VulkanBuffer *host_vertex_buffer)
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

    Int vertex_buffer_length = sizeof(MenuVertex) * MAX_MENU_VERTEX_COUNT;
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
        BitmapFont *font = &menu_fonts[font_i];

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

struct MenuLayout
{
    Vec2 player_pos[GameSide::count];
    Vec2 player_end_pos[GameSide::count];
};

Str player_option[GameSide::count] = {str("white"), str("black")};
Vec2 player_option_pos[GameSide::count] = {Vec2{0.0, -0.78}, Vec2{0.35, -0.78}};
MenuLayout get_menu_layout(BitmapFont *menu_fonts, Int window_width, Int window_height)
{
    MenuLayout menu_layout;
    for (GameSideEnum side = 0; side < GameSide::count; side++)
    {
        menu_layout.player_pos[side] = player_option_pos[side];
        Vec2 size = get_string_texture_size(&menu_fonts[MENU_MEDIUM_FONT], player_option[side], window_width, window_height);
        menu_layout.player_end_pos[side].x = player_option_pos[side].x + size.x;
        menu_layout.player_end_pos[side].y = player_option_pos[side].y + size.y;
    }
    return menu_layout;
}

struct MenuDrawState
{
    BitmapFont *fonts;
    Int window_width;
    Int window_height;
    MenuVertex *menu_vertex;
};

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

Vec2 draw_string(MenuDrawState *draw_state, Str string, Vec2 pos, Real alpha, Int font_type)
{
    BitmapFont *font = &draw_state->fonts[font_type];
    for (Int8 char_i = 0; char_i < string.count; char_i++)
    {
        CharTextureInfo texture_info = get_char_texture_info(font, string[char_i], draw_state->window_width, draw_state->window_height);
        draw_char(&texture_info, draw_state->menu_vertex, pos, alpha, font_type);
        pos.x += texture_info.advance;
        draw_state->menu_vertex += 6;
    }
    pos.y += get_line_texture_info(font, draw_state->window_width, draw_state->window_height);
    return pos;
}

Void draw_underline(MenuDrawState *draw_state, Vec2 pos, Real width, Real height, Real alpha, Real alpha_right)
{
    MenuVertex *menu_vertex = draw_state->menu_vertex;
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

    draw_state->menu_vertex += 6;
}

struct MenuState
{
    GameSideEnum hovered_player;
    GameSideEnum selected_player;
};

Int draw_menu(BitmapFont *menu_fonts, MenuState *state, Real alpha, Int window_width, Int window_height, VulkanBuffer *vertex_buffer)
{
    MenuDrawState draw_state = {};
    draw_state.menu_vertex = (MenuVertex *)vertex_buffer->data;
    draw_state.window_width = window_width;
    draw_state.window_height = window_height;
    draw_state.fonts = menu_fonts;

    Real select_line_width = 0.01;

    Str player = str("Player");
    Vec2 player_pos = Vec2{-0.8, -0.8};
    draw_string(&draw_state, player, player_pos, alpha, MENU_LARGE_FONT);

    for (GameSideEnum side = 0; side < GameSide::count; side++)
    {
        Vec2 start_pos = player_option_pos[side];
        Vec2 end_pos = draw_string(&draw_state, player_option[side], start_pos, alpha, MENU_MEDIUM_FONT);

        if (state->selected_player == side)
        {
            draw_underline(&draw_state, Vec2{start_pos.x, end_pos.y}, end_pos.x - start_pos.x, select_line_width, alpha, alpha);
        }
        if (state->hovered_player == side)
        {
            draw_underline(&draw_state, Vec2{start_pos.x, end_pos.y}, end_pos.x - start_pos.x, select_line_width, alpha * 0.5, alpha * 0.5);
        }
    }

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
        draw_string(&draw_state, rules[rule_i], rule_pos[rule_i], alpha, MENU_SMALL_FONT);
    }

    draw_underline(&draw_state, Vec2{-0.8, 0.45}, 1.5, 0.0025, alpha, alpha * 0.1);

    Int vertex_count = draw_state.menu_vertex - (MenuVertex *)vertex_buffer->data;
    ASSERT(vertex_count <= MAX_MENU_VERTEX_COUNT);
    return vertex_count;
}

Int draw_game_end(BitmapFont *menu_fonts, GameEndEnum game_end, Real alpha, Int window_width, Int window_height, VulkanBuffer *vertex_buffer)
{
    MenuDrawState draw_state = {};
    draw_state.menu_vertex = (MenuVertex *)vertex_buffer->data;
    draw_state.window_width = window_width;
    draw_state.window_height = window_height;
    draw_state.fonts = menu_fonts;

    ASSERT(game_end != GameEnd::none);
    Str title;
    switch (game_end)
    {
    case GameEnd::win:
    {
        title = str("Win");
    }
    break;

    case GameEnd::lose:
    {
        title = str("Lose");
    }
    break;

    case GameEnd::draw:
    {
        title = str("Draw");
    }
    break;

    default:
    {
        ASSERT(false);
    }
    break;
    }

    Vec2 size = get_string_texture_size(&menu_fonts[MENU_LARGE_FONT], title, window_width, window_height);
    Vec2 pos = {0 - size.x / 2, 0 - size.y / 2};
    draw_string(&draw_state, title, pos, alpha, MENU_LARGE_FONT);

    Int vertex_count = draw_state.menu_vertex - (MenuVertex *)vertex_buffer->data;
    ASSERT(vertex_count <= MAX_MENU_VERTEX_COUNT);
    return vertex_count;
}
