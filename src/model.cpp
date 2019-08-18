#pragma once

#include "util.cpp"
#include "math.cpp"

struct Model
{
    Int4 vertices_count;
    Int4 indices_count;
    Vec3 *vertices_data;
    UInt4 *indices_data;
};

Bool deserialise_model(Str buffer, OUT Model *model)
{
    if (buffer.length < 4)
    {
        return false;
    }
    model->vertices_count = *(Int4 *)buffer.data;

    Int vertices_data_start = 4;
    Int vertices_data_length = sizeof(Vec3) * model->vertices_count;
    if (buffer.length < vertices_data_start + vertices_data_length)
    {
        return false;
    }

    Int indices_count_start = vertices_data_start + vertices_data_length;
    if (buffer.length < indices_count_start + 4)
    {
        return false;
    }
    model->indices_count = *(Int4 *)(buffer.data + indices_count_start);

    Int indices_data_start = indices_count_start + 4;
    Int indices_data_length = sizeof(Int4) * model->indices_count;
    if (buffer.length != indices_data_start + indices_data_length)
    {
        return false;
    }

    model->vertices_data = (Vec3 *)malloc(vertices_data_length);
    memcpy(model->vertices_data, buffer.data + vertices_data_start, vertices_data_length);
    model->indices_data = (UInt4 *)malloc(indices_data_length);
    memcpy(model->indices_data, buffer.data + indices_data_start, indices_data_length);

    return true;
}
