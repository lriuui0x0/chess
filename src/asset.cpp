#pragma once

#include "util.cpp"
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
    UInt32 *indices_data;
};

Bool deserialise_mesh(Str buffer, OUT Mesh *mesh)
{
    if (buffer.length < 4)
    {
        return false;
    }
    mesh->vertex_count = *(Int32 *)buffer.data;

    Int vertex_data_start = 4;
    Int vertex_data_length = sizeof(Vertex) * mesh->vertex_count;
    if (buffer.length < vertex_data_start + vertex_data_length)
    {
        return false;
    }

    Int index_count_start = vertex_data_start + vertex_data_length;
    if (buffer.length < index_count_start + 4)
    {
        return false;
    }
    mesh->index_count = *(Int32 *)(buffer.data + index_count_start);

    Int index_data_start = index_count_start + 4;
    Int index_data_length = sizeof(Int32) * mesh->index_count;
    if (buffer.length != index_data_start + index_data_length)
    {
        return false;
    }

    mesh->vertices_data = (Vertex *)malloc(vertex_data_length);
    memcpy(mesh->vertices_data, buffer.data + vertex_data_start, vertex_data_length);
    mesh->indices_data = (UInt32 *)malloc(index_data_length);
    memcpy(mesh->indices_data, buffer.data + index_data_start, index_data_length);

    return true;
}

struct FontCharPos
{
    Int16 offset;
    Int16 width;
};

struct Font
{
    Int8 start_char;
    Int8 num_char;
    Int16 width;
    Int16 height;
    Int16 line_height;
    FontCharPos *pos;
    UInt32 *data;
};

Bool deserialise_font(Str buffer, OUT Font *font)
{
    if (buffer.length < 8)
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
    font->line_height = *(Int16 *)(buffer.data + pos);
    pos += 2;

    Int pos_data_length = sizeof(FontCharPos) * font->num_char;
    Int image_data_length = sizeof(UInt32) * font->width * font->height;
    if (buffer.length != pos + pos_data_length + image_data_length)
    {
        return false;
    }

    font->pos = (FontCharPos *) malloc(pos_data_length);
    memcpy(font->pos, buffer.data + pos, pos_data_length);
    pos += pos_data_length;

    font->data = (UInt32 *) malloc(image_data_length);
    memcpy(font->data, buffer.data + pos, image_data_length);

    return true;
}
