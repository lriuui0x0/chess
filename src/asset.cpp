#pragma once

#include "../lib/util.hpp"
#include "../lib/vulkan.hpp"
#include "math.cpp"
#include "collision.cpp"
#include "game.cpp"

struct Vertex
{
    Vec3 pos;
    Vec3 normal;
    Vec3 color;
    Vec2 uv;
};

struct Mesh
{
    Int32 vertex_count;
    Vertex *vertices_data;
    Int32 index_count;
    Int32 *indices_data;
    CollisionBox collision_box;
    Int32 collision_hull_vertex_count;
    Vec3 *collision_hull_vertex_data;

    Int vertex_offset;
    Int index_offset;
};

Bool deserialise_mesh(Str buffer, Mesh *mesh)
{
    if (buffer.count < 4)
    {
        return false;
    }
    mesh->vertex_count = *(Int32 *)buffer.data;

    Int vertex_data_start = 4;
    Int vertex_data_length = sizeof(Vertex) * mesh->vertex_count;
    if (buffer.count < vertex_data_start + vertex_data_length)
    {
        return false;
    }

    Int index_count_start = vertex_data_start + vertex_data_length;
    if (buffer.count < index_count_start + 4)
    {
        return false;
    }
    mesh->index_count = *(Int32 *)(buffer.data + index_count_start);

    Int index_data_start = index_count_start + 4;
    Int index_data_length = sizeof(Int32) * mesh->index_count;
    if (buffer.count < index_data_start + index_data_length)
    {
        return false;
    }

    Int collision_box_center_start = index_data_start + index_data_length;
    if (buffer.count < collision_box_center_start + (Int)sizeof(Vec3))
    {
        return false;
    }
    mesh->collision_box.center = *(Vec3 *)(buffer.data + collision_box_center_start);

    Int collision_box_radius_start = collision_box_center_start + (Int)sizeof(Vec3);
    if (buffer.count < collision_box_radius_start + (Int)sizeof(Vec3))
    {
        return false;
    }
    mesh->collision_box.radius = *(Vec3 *)(buffer.data + collision_box_radius_start);

    Int collision_hull_vertex_count_start = collision_box_radius_start + sizeof(Vec3);
    if (buffer.count < collision_hull_vertex_count_start + 4)
    {
        return false;
    }
    mesh->collision_hull_vertex_count = *(Int32 *)(buffer.data + collision_hull_vertex_count_start);

    Int hull_vertex_data_start = collision_hull_vertex_count_start + 4;
    Int hull_vertex_data_length = sizeof(Vec3) * mesh->collision_hull_vertex_count;
    if (buffer.count != hull_vertex_data_start + hull_vertex_data_length)
    {
        return false;
    }

    mesh->vertices_data = (Vertex *)malloc(vertex_data_length);
    memcpy(mesh->vertices_data, buffer.data + vertex_data_start, vertex_data_length);
    mesh->indices_data = (Int32 *)malloc(index_data_length);
    memcpy(mesh->indices_data, buffer.data + index_data_start, index_data_length);
    mesh->collision_hull_vertex_data = (Vec3 *)malloc(hull_vertex_data_length);
    memcpy(mesh->collision_hull_vertex_data, buffer.data + hull_vertex_data_start, hull_vertex_data_length);

    return true;
}

#define MENU_LARGE_FONT (0)
#define MENU_MEDIUM_FONT (1)
#define MENU_SMALL_FONT (2)
#define MENU_FONT_COUNT (3)

struct FontCharHeader
{
    Int32 offset;
    Int32 width;
    Int32 advance;
    Int32 left_bearing;
};

struct Font
{
    Int32 start_char;
    Int32 num_char;
    Int32 width;
    Int32 height;
    Int32 line_advance;
    FontCharHeader *pos;
    UInt32 *data;
};

Bool deserialise_font(Str buffer, Font *font)
{
    if (buffer.count < 10)
    {
        return false;
    }

    Int pos = 0;
    font->start_char = *(Int32 *)(buffer.data + pos);
    pos += 4;
    font->num_char = *(Int32 *)(buffer.data + pos);
    pos += 4;
    font->width = *(Int32 *)(buffer.data + pos);
    pos += 4;
    font->height = *(Int32 *)(buffer.data + pos);
    pos += 4;
    font->line_advance = *(Int32 *)(buffer.data + pos);
    pos += 4;

    Int pos_data_length = sizeof(FontCharHeader) * font->num_char;
    Int image_data_length = sizeof(UInt32) * font->width * font->height;
    if (buffer.count != pos + pos_data_length + image_data_length)
    {
        return false;
    }

    font->pos = (FontCharHeader *)malloc(pos_data_length);
    memcpy(font->pos, buffer.data + pos, pos_data_length);
    pos += pos_data_length;

    font->data = (UInt32 *)malloc(image_data_length);
    memcpy(font->data, buffer.data + pos, image_data_length);

    return true;
}

struct CharTextureInfo
{
    Real width;
    Real height;
    Real left_bearing;
    Real advance;
    Real uv_x_min;
    Real uv_x_max;
};

CharTextureInfo get_char_texture_info(Font *font, Int8 character, Int window_width, Int window_height)
{
    ASSERT(character >= font->start_char && character < font->start_char + font->num_char);
    Int8 char_index = character - font->start_char;
    FontCharHeader *char_header = &font->pos[char_index];

    CharTextureInfo texture_info;
    texture_info.width = (Real)char_header->width / window_width * 2;
    texture_info.height = (Real)font->height / window_height * 2;
    texture_info.left_bearing = (Real)char_header->left_bearing / window_width * 2;
    texture_info.advance = (Real)char_header->advance / window_width * 2;
    texture_info.uv_x_min = (Real)char_header->offset / font->width;
    texture_info.uv_x_max = (Real)(char_header->offset + char_header->width) / font->width;
    return texture_info;
}

Real get_line_texture_info(Font *font, Int window_width, Int window_height)
{
    Real line_advance = (Real)font->line_advance / window_height * 2;
    return line_advance;
}

Vec2 get_string_texture_size(Font *font, Str string, Int window_width, Int window_height)
{
    Vec2 size;
    size.x = 0;
    for (Int8 char_i = 0; char_i < string.count; char_i++)
    {
        CharTextureInfo texture_info = get_char_texture_info(font, string[char_i], window_width, window_height);
        size.x += texture_info.advance;
    }
    size.y = get_line_texture_info(font, window_width, window_height);
    return size;
}

struct Image
{
    Int width;
    Int height;
    UInt8 *data;

    VkImage image;
    VkSampler sampler;
    VkDescriptorSet descriptor_set;
};

Bool deserialise_image(Str buffer, Image *image)
{
    if (buffer.count < 8)
    {
        return false;
    }

    Int pos = 0;
    image->width = *(Int *)(buffer.data + pos);
    pos += 4;
    image->height = *(Int *)(buffer.data + pos);
    pos += 4;

    Int image_data_length = image->width * image->height * sizeof(UInt8);
    if (buffer.count != pos + image_data_length)
    {
        return false;
    }
    image->data = (UInt8 *)malloc(image_data_length);
    memcpy(image->data, buffer.data + pos, image_data_length);
    return true;
}

struct Sound
{
    Int32 sample_count;
    Int32 channel_count;
    Int32 sample_byte_count;
    UInt8 *data;
};

Bool deserialise_sound(Str buffer, Sound *sound)
{
    if (buffer.count < 12)
    {
        return false;
    }

    Int pos = 0;
    sound->sample_count = *(Int32 *)(buffer.data + pos);
    pos += 4;
    sound->channel_count = *(Int32 *)(buffer.data + pos);
    pos += 4;
    sound->sample_byte_count = *(Int *)(buffer.data + pos);
    pos += 4;

    Int sound_data_length = sound->sample_count * sound->channel_count * sound->sample_byte_count * sizeof(UInt8);
    if (buffer.count != pos + sound_data_length)
    {
        return false;
    }
    sound->data = (UInt8 *)malloc(sound_data_length);
    memcpy(sound->data, buffer.data + pos, sound_data_length);
    return true;
}

Bool deserialise_bit_board_table(Str buffer, BitBoardTable *table)
{
    Int move_data_length;
    Int pos = 0;
    for (Int square = 0; square < 64; square++)
    {
        SlidingPieceTableSquare *table_square = &table->rook_table.board[square];
        if (buffer.count < pos + 8)
        {
            return false;
        }
        table_square->blocker_mask = *(BitBoard *)(buffer.data + pos);
        pos += 8;

        if (buffer.count < pos + 8)
        {
            return false;
        }
        table_square->blocker_bit_count = *(UInt64 *)(buffer.data + pos);
        pos += 8;

        if (buffer.count < pos + 8)
        {
            return false;
        }
        table_square->magic = *(UInt64 *)(buffer.data + pos);
        pos += 8;

        move_data_length = sizeof(BitBoard) * (1 << table_square->blocker_bit_count);
        if (buffer.count < pos + move_data_length)
        {
            return false;
        }
        table_square->move = (BitBoard *)malloc(move_data_length);
        memcpy(table_square->move, buffer.data + pos, move_data_length);
        pos += move_data_length;
    }

    move_data_length = sizeof(BitBoard) * 64;
    if (buffer.count < pos + move_data_length)
    {
        return false;
    }
    memcpy(&table->knight_table.move, buffer.data + pos, move_data_length);
    pos += move_data_length;

    for (Int square = 0; square < 64; square++)
    {
        SlidingPieceTableSquare *table_square = &table->bishop_table.board[square];
        if (buffer.count < pos + 8)
        {
            return false;
        }
        table_square->blocker_mask = *(BitBoard *)(buffer.data + pos);
        pos += 8;

        if (buffer.count < pos + 8)
        {
            return false;
        }
        table_square->blocker_bit_count = *(UInt64 *)(buffer.data + pos);
        pos += 8;

        if (buffer.count < pos + 8)
        {
            return false;
        }
        table_square->magic = *(UInt64 *)(buffer.data + pos);
        pos += 8;

        move_data_length = sizeof(BitBoard) * (1 << table_square->blocker_bit_count);
        if (buffer.count < pos + move_data_length)
        {
            return false;
        }
        table_square->move = (BitBoard *)malloc(move_data_length);
        memcpy(table_square->move, buffer.data + pos, move_data_length);
        pos += move_data_length;
    }

    move_data_length = sizeof(BitBoard) * 64;
    if (buffer.count < pos + move_data_length)
    {
        return false;
    }
    memcpy(&table->king_table.move, buffer.data + pos, move_data_length);
    pos += move_data_length;

    if (buffer.count != pos)
    {
        return false;
    }
    return true;
}

struct AssetStore
{
    Mesh board_mesh;
    Mesh piece_meshes[GameSide::count][GamePieceType::count];
    Image board_light_map;
    Image piece_light_maps[GamePieceType::count];
    Font debug_font;
    Font menu_fonts[MENU_FONT_COUNT];
    Sound sound_click_high;
    Sound sound_click_low;
    Sound sound_move;
    Sound sound_error;
    BitBoardTable *bit_board_table;
};

Bool load_asset(AssetStore *asset_store)
{
    Str file_contents;
    if (read_file("../asset/board.asset", &file_contents))
    {
        if (!deserialise_mesh(file_contents, &asset_store->board_mesh))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    CStr piece_meshes_paths[GameSide::count][GamePieceType::count] = {
        {
            "../asset/pawn_white.asset",
            "../asset/knight_white.asset",
            "../asset/bishop_white.asset",
            "../asset/rook_white.asset",
            "../asset/queen_white.asset",
            "../asset/king_white.asset",
        },
        {
            "../asset/pawn_black.asset",
            "../asset/knight_black.asset",
            "../asset/bishop_black.asset",
            "../asset/rook_black.asset",
            "../asset/queen_black.asset",
            "../asset/king_black.asset",
        }};
    for (GameSideEnum side = 0; side < GameSide::count; side++)
    {
        for (GamePieceTypeEnum piece_type = 0; piece_type < GamePieceType::count; piece_type++)
        {
            if (read_file(piece_meshes_paths[side][piece_type], &file_contents))
            {
                if (!deserialise_mesh(file_contents, &asset_store->piece_meshes[side][piece_type]))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }

    if (read_file("../asset/board_lightmap.asset", &file_contents))
    {
        if (!deserialise_image(file_contents, &asset_store->board_light_map))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    CStr piece_light_maps_paths[GamePieceType::count] = {
        "../asset/pawn_lightmap.asset",
        "../asset/knight_lightmap.asset",
        "../asset/bishop_lightmap.asset",
        "../asset/rook_lightmap.asset",
        "../asset/queen_lightmap.asset",
        "../asset/king_lightmap.asset",
    };
    for (GamePieceTypeEnum piece_type = 0; piece_type < GamePieceType::count; piece_type++)
    {
        if (read_file(piece_light_maps_paths[piece_type], &file_contents))
        {
            if (!deserialise_image(file_contents, &asset_store->piece_light_maps[piece_type]))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    CStr sound_files[4] = {"../asset/sound_click_high.asset", "../asset/sound_click_low.asset", "../asset/sound_move.asset", "../asset/sound_error.asset"};
    Sound *sounds[4] = {&asset_store->sound_click_high, &asset_store->sound_click_low, &asset_store->sound_move, &asset_store->sound_error};
    for (Int i = 0; i < 4; i++)
    {
        if (read_file(sound_files[i], &file_contents))
        {
            if (!deserialise_sound(file_contents, sounds[i]))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    if (read_file("../asset/debug_font.asset", &file_contents))
    {
        if (!deserialise_font(file_contents, &asset_store->debug_font))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    CStr menu_font_files[MENU_FONT_COUNT] = {"../asset/menu_large_font.asset", "../asset/menu_medium_font.asset", "../asset/menu_small_font.asset"};
    for (Int font_i = 0; font_i < MENU_FONT_COUNT; font_i++)
    {
        if (read_file(menu_font_files[font_i], &file_contents))
        {
            if (!deserialise_font(file_contents, &asset_store->menu_fonts[font_i]))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    if (read_file("../asset/bitboard.asset", &file_contents))
    {
        asset_store->bit_board_table = (BitBoardTable *)malloc(sizeof(BitBoardTable));
        if (!asset_store->bit_board_table || !deserialise_bit_board_table(file_contents, asset_store->bit_board_table))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}
