#pragma once

#include "../src/util.cpp"

struct Bitmap
{
    Int width;
    Int height;
    UInt32 *data;
};

void draw_line(Bitmap *bitmap, Real fx0, Real fy0, Real fx1, Real fy1, UInt32 color)
{
    Int x0 = round(fx0);
    Int y0 = round(fy0);
    Int x1 = round(fx1);
    Int y1 = round(fy1);

    Real dx = (x1 - x0) / 1000.0f;
    Real dy = (y1 - y0) / 1000.0f;
    for (Int i = 0; i < 1000; i++)
    {
        Int x = x0 + i * dx;
        Int y = y0 + i * dy;
        *(bitmap->data + bitmap->width * y + x) = color;
    }
}

void write_bitmap(Str filename, Bitmap *bitmap)
{
    FILE *file_handle = fopen((RawStr)filename.data, "wb");
    assert(file_handle);

    UInt8 bf_type[2] = {'B', 'M'};
    fwrite(&bf_type, 2, 1, file_handle);
    UInt32 bf_size = 14 + 40 + 4 * bitmap->width * bitmap->height;
    fwrite(&bf_size, 4, 1, file_handle);
    UInt16 bf_reserved1 = 0;
    fwrite(&bf_reserved1, 2, 1, file_handle);
    UInt16 bf_reserved2 = 0;
    fwrite(&bf_reserved2, 2, 1, file_handle);
    UInt32 bf_off_bits = 14 + 40;
    fwrite(&bf_off_bits, 4, 1, file_handle);

    UInt32 bi_size = 40;
    fwrite(&bi_size, 4, 1, file_handle);
    UInt32 bi_width = bitmap->width;
    fwrite(&bi_width, 4, 1, file_handle);
    UInt32 bi_height = bitmap->height;
    fwrite(&bi_height, 4, 1, file_handle);
    UInt16 bi_planes = 1;
    fwrite(&bi_planes, 2, 1, file_handle);
    UInt16 bi_bit_count = 32;
    fwrite(&bi_bit_count, 2, 1, file_handle);
    UInt32 bi_compression = 0;
    fwrite(&bi_compression, 4, 1, file_handle);
    UInt32 bi_size_image = 0;
    fwrite(&bi_size_image, 4, 1, file_handle);
    UInt32 bi_x_pels_per_meter = 0;
    fwrite(&bi_x_pels_per_meter, 4, 1, file_handle);
    UInt32 bi_y_pels_per_meter = 0;
    fwrite(&bi_y_pels_per_meter, 4, 1, file_handle);
    UInt32 bi_clr_used = 0;
    fwrite(&bi_clr_used, 4, 1, file_handle);
    UInt32 bi_clr_important = 0;
    fwrite(&bi_clr_important, 4, 1, file_handle);

    UInt32 *row = bitmap->data;
    for (Int y = 0; y < bitmap->height; y++)
    {
        for (Int x = 0; x < bitmap->width; x++)
        {
            UInt32 color = row[x];
            fwrite(&color, 4, 1, file_handle);
        }
        row += bitmap->width;
    }

    assert(fclose(file_handle) == 0);
}
