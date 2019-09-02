#pragma once

#include "vulkan.cpp"
#include "asset.cpp"

struct DebugUIDrawState
{
    Font *font;
    Int window_width;
    Int window_height;
    VulkanBuffer *vertex_buffer;

    Int character_count;
    Vec2 pos;
    Vec2 line_pos;
    Int indent;
};

DebugUIDrawState create_debug_ui_draw_state(Font *font, Int window_width, Int window_height, VulkanBuffer *vertex_buffer, Vec2 initial_pos)
{
    DebugUIDrawState result;
    result.font = font;
    result.window_width = window_width;
    result.window_height = window_height;
    result.vertex_buffer = vertex_buffer;
    result.pos = initial_pos;
    result.line_pos = initial_pos;
    result.indent = 0;
    result.character_count = 0;
    return result;
}

void debug_ui_draw_char(DebugUIDrawState *draw_state, Int8 character)
{
    Font *font = draw_state->font;
    if (character == ' ')
    {
        draw_state->pos.x += (Real)font->whitespace_advance / draw_state->window_width;
        return;
    }

    assert(character >= font->start_char && character < font->start_char + font->num_char);

    Int8 char_index = character - font->start_char;
    FontCharHeader *char_header = &font->pos[char_index];

    Real width = (Real)char_header->width / draw_state->window_width;
    Real height = (Real)font->height / draw_state->window_height;
    Real left_bearing = (Real)char_header->left_bearing / draw_state->window_width;
    Real advance = (Real)char_header->advance / draw_state->window_width;
    Vec2 char_pos = {draw_state->pos.x + left_bearing, draw_state->pos.y};

    Real texture_coord_x_min = (Real)char_header->offset / font->width;
    Real texture_coord_x_max = (Real)(char_header->offset + char_header->width) / font->width;

    DebugUIVertex *debug_ui_vertex = (DebugUIVertex *)draw_state->vertex_buffer->data + draw_state->character_count++ * 6;
    debug_ui_vertex[0].pos = char_pos;
    debug_ui_vertex[0].texture_coord = {texture_coord_x_min, 0};

    debug_ui_vertex[1].pos = {char_pos.x, char_pos.y + height};
    debug_ui_vertex[1].texture_coord = {texture_coord_x_min, 1};

    debug_ui_vertex[2].pos = {char_pos.x + width, char_pos.y};
    debug_ui_vertex[2].texture_coord = {texture_coord_x_max, 0};

    debug_ui_vertex[3].pos = {char_pos.x + width, char_pos.y + height};
    debug_ui_vertex[3].texture_coord = {texture_coord_x_max, 1};

    debug_ui_vertex[4].pos = {char_pos.x + width, char_pos.y};
    debug_ui_vertex[4].texture_coord = {texture_coord_x_max, 0};

    debug_ui_vertex[5].pos = {char_pos.x, char_pos.y + height};
    debug_ui_vertex[5].texture_coord = {texture_coord_x_min, 1};

    draw_state->pos.x += advance;
}

void debug_ui_draw_indent(DebugUIDrawState *draw_state, Int indent)
{
    draw_state->indent += indent;
    Real indent_advance = 2 * indent * ((Real)draw_state->font->whitespace_advance / draw_state->window_width);
    draw_state->pos.x += indent_advance;
    draw_state->line_pos.x += indent_advance;
}

void debug_ui_draw_newline(DebugUIDrawState *draw_state)
{
    Real line_advance = (Real)draw_state->font->line_advance / draw_state->window_height;
    draw_state->line_pos.y += line_advance;
    draw_state->pos = draw_state->line_pos;
}

void debug_ui_draw_str(DebugUIDrawState *draw_state, Str string)
{
    for (Int i = 0; i < string.length; i++)
    {
        debug_ui_draw_char(draw_state, string[i]);
    }
}

void debug_ui_draw_real(DebugUIDrawState *draw_state, Real real)
{
    Bool is_negative = false;
    if (real < 0)
    {
        is_negative = true;
        real = -real;
    }

    Int integer = real;
    Real fraction = real - integer;

    Int8 integer_char_count = 0;
    Int8 integer_chars[16];
    do 
    {
        integer_chars[integer_char_count++] = integer % 10 + '0';
        integer /= 10;
    } while (integer);

    Int8 fraction_chars[6];
    for (Int i = 0; i < 6; i++)
    {
        fraction *= 10;
        Int digit = (Int)fraction;
        fraction_chars[i] = digit + '0';
        fraction -= digit;
    }

    if (is_negative)
    {
        debug_ui_draw_char(draw_state, '-');
    }
    for (Int i = integer_char_count - 1; i >= 0; i--)
    {
        debug_ui_draw_char(draw_state, integer_chars[i]);
    }
    debug_ui_draw_char(draw_state, '.');
    for (Int i = 0; i < 6; i++)
    {
        debug_ui_draw_char(draw_state, fraction_chars[i]);
    }
}

void debug_ui_draw_vec3(DebugUIDrawState *draw_state, Vec3 vec3)
{
    debug_ui_draw_real(draw_state, vec3.x);
    debug_ui_draw_str(draw_state, wrap_str(", "));
    debug_ui_draw_real(draw_state, vec3.y);
    debug_ui_draw_str(draw_state, wrap_str(", "));
    debug_ui_draw_real(draw_state, vec3.z);
}
