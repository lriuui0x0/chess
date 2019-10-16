#pragma once

#include "../lib/util.hpp"
#include <cstring>
#include <cstdio>

struct Bitmap
{
    Int width;
    Int height;
    UInt32 *data;
};

Bitmap create_bitmap(Int width, Int height)
{
    Bitmap bitmap;
    bitmap.width = width;
    bitmap.height = height;
    bitmap.data = (UInt32 *)malloc(sizeof(UInt32) * bitmap.width * bitmap.height);
    memset(bitmap.data, 0, sizeof(UInt32) * bitmap.width * bitmap.height);
    return bitmap;
}

void set_pixel(Bitmap *bitmap, Int x, Int y, UInt32 color)
{
    if (x >= 0 && x < bitmap->width && y >= 0 && y < bitmap->height)
    {
        *(bitmap->data + bitmap->width * y + x) = color;
    }
}

void draw_line(Bitmap *bitmap, Real fx0, Real fy0, Real fx1, Real fy1)
{
    ASSERT(fx0 >= 0 && fx0 <= bitmap->width);
    ASSERT(fx1 >= 0 && fx1 <= bitmap->width);
    ASSERT(fy0 >= 0 && fy0 <= bitmap->height);
    ASSERT(fy1 >= 0 && fy1 <= bitmap->height);

    Real fdx = fx1 - fx0;
    Real fdy = fy1 - fy0;

    if (fabs(fdy) <= fabs(fdx))
    {
        if (fx1 < fx0)
        {
            swap(&fx0, &fx1);
            swap(&fy0, &fy1);
        }
        Int sign = fy1 > fy0 ? 1 : -1;

        Int x0 = round(fx0);
        Int x1 = round(fx1);
        Int y0 = round(fy0);

        Real delta = abs(fdy / fdx);
        Real error = sign * (fy0 - y0);
        Int y = y0;
        for (Int x = x0; x <= x1; x++)
        {
            set_pixel(bitmap, x, y, 0);

            error += delta;
            if (error > 0.5)
            {
                y += sign;
                error -= 1;
            }
        }
    }
    else
    {
        if (fy1 < fy0)
        {
            swap(&fy0, &fy1);
            swap(&fx0, &fx1);
        }
        Int sign = fx1 > fx0 ? 1 : -1;

        Int y0 = round(fy0);
        Int y1 = round(fy1);
        Int x0 = round(fx0);

        Real delta = abs(fdx / fdy);
        Real error = sign * (fx0 - x0);
        Int x = x0;
        for (Int y = y0; y <= y1; y++)
        {
            set_pixel(bitmap, x, y, 0);

            error += delta;
            if (error > 0.5)
            {
                x += sign;
                error -= 1;
            }
        }
    }
}

void write_bitmap(Str filename, Bitmap *bitmap)
{
    FILE *file_handle = fopen((CStr)filename.data, "wb");
    ASSERT(file_handle);

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

    ASSERT(fclose(file_handle) == 0);
}
