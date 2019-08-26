#pragma once

#include "util.cpp"
#include "math.cpp"

struct Vertex
{
    Vec3 pos;
    Vec3 normal;
};

struct Mesh
{
    Int32 vertex_count;
    Int32 index_count;
    Vertex *vertices_data;
    UInt32 *indices_data;
};

Bool deserialise_model(Str buffer, OUT Mesh *model)
{
    if (buffer.length < 4)
    {
        return false;
    }
    model->vertex_count = *(Int32 *)buffer.data;

    Int vertex_data_start = 4;
    Int vertex_data_length = sizeof(Vertex) * model->vertex_count;
    if (buffer.length < vertex_data_start + vertex_data_length)
    {
        return false;
    }

    Int index_count_start = vertex_data_start + vertex_data_length;
    if (buffer.length < index_count_start + 4)
    {
        return false;
    }
    model->index_count = *(Int32 *)(buffer.data + index_count_start);

    Int index_data_start = index_count_start + 4;
    Int index_data_length = sizeof(Int32) * model->index_count;
    if (buffer.length != index_data_start + index_data_length)
    {
        return false;
    }

    model->vertices_data = (Vertex *)malloc(vertex_data_length);
    memcpy(model->vertices_data, buffer.data + vertex_data_start, vertex_data_length);
    model->indices_data = (UInt32 *)malloc(index_data_length);
    memcpy(model->indices_data, buffer.data + index_data_start, index_data_length);

    return true;
}
