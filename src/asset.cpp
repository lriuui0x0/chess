#pragma once

#include "../lib/util.hpp"
#include "math.cpp"

struct Vertex
{
    Vec3 pos;
    Vec3 normal;
    Vec3 color;
};

struct Mesh
{
    Int32 vertex_count;
    Int32 index_count;
    Vertex *vertices_data;
    Int32 *indices_data;
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
    if (buffer.count != index_data_start + index_data_length)
    {
        return false;
    }

    mesh->vertices_data = (Vertex *)malloc(vertex_data_length);
    memcpy(mesh->vertices_data, buffer.data + vertex_data_start, vertex_data_length);
    mesh->indices_data = (Int32 *)malloc(index_data_length);
    memcpy(mesh->indices_data, buffer.data + index_data_start, index_data_length);

    return true;
}

struct FontCharHeader
{
    Int16 offset;
    Int16 width;
    Int16 advance;
    Int16 left_bearing;
};

struct Font
{
    Int8 start_char;
    Int8 num_char;
    Int16 width;
    Int16 height;
    Int16 line_advance;
    Int16 whitespace_advance;
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
    font->start_char = *(Int8 *)(buffer.data + pos);
    pos++;
    font->num_char = *(Int8 *)(buffer.data + pos);
    pos++;
    font->width = *(Int16 *)(buffer.data + pos);
    pos += 2;
    font->height = *(Int16 *)(buffer.data + pos);
    pos += 2;
    font->line_advance = *(Int16 *)(buffer.data + pos);
    pos += 2;
    font->whitespace_advance = *(Int16 *)(buffer.data + pos);
    pos += 2;

    Int pos_data_length = sizeof(FontCharHeader) * font->num_char;
    Int image_data_length = sizeof(UInt32) * font->width * font->height;
    if (buffer.count != pos + pos_data_length + image_data_length)
    {
        return false;
    }

    font->pos = (FontCharHeader *) malloc(pos_data_length);
    memcpy(font->pos, buffer.data + pos, pos_data_length);
    pos += pos_data_length;

    font->data = (UInt32 *) malloc(image_data_length);
    memcpy(font->data, buffer.data + pos, image_data_length);

    return true;
}
