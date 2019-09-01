#pragma once

#include "../src/util.cpp"

// The algorithm in this file is horrible. Should use proper scanline fill instead.

struct Bitmap
{
    Int width;
    Int height;
    UInt32 *data;
};

void set_pixel(Bitmap *bitmap, Int x, Int y, UInt32 color)
{
    if (x >= 0 && x < bitmap->width && y >= 0 && y < bitmap->height)
    {
        *(bitmap->data + bitmap->width * y + x) = color;
    }
}

UInt32 get_pixel(Bitmap *bitmap, Int x, Int y)
{
    UInt32 result = *(bitmap->data + bitmap->width * y + x);
    return result;
}

void draw_line(Bitmap *bitmap, Real fx0, Real fy0, Real fx1, Real fy1)
{
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

struct Point
{
    Int x;
    Int y;
};

Int inc_x[4] = {1, 0, -1, 0};
Int inc_y[4] = {0, 1, 0, -1};

Bool get_visited(Bitmap *bitmap, Bool *visited, Int x, Int y)
{
    Bool result = *(visited + bitmap->width * y + x);
    return result;
}

void fill_black(Bitmap *bitmap, Int x, Int y, Bool *visited)
{
    Array<Point> queue = create_array<Point>();
    Int head = 0;
    *queue.push() = {x, y};
    *(visited + bitmap->width * y + x) = true;

    while (queue.length - head > 0)
    {
        Int x = queue[head].x;
        Int y = queue[head].y;
        head++;

        for (Int i = 0; i < 4; i++)
        {
            Int new_x = x + inc_x[i];
            Int new_y = y + inc_y[i];

            if (new_x >= 0 && new_x < bitmap->width && new_y >= 0 && new_y < bitmap->height)
            {
                UInt32 pixel_color = get_pixel(bitmap, new_x, new_y);
                Bool pixel_visited = get_visited(bitmap, visited, new_x, new_y);

                if (!pixel_visited)
                {
                    *(visited + bitmap->width * new_y + new_x) = true;

                    if (pixel_color == 0xffffffff)
                    {
                        Point *queue_point = queue.push();
                        queue_point->x = new_x;
                        queue_point->y = new_y;

                        set_pixel(bitmap, new_x, new_y, 0);
                    }
                }
            }
        }
    }

    destroy_array(queue);
}

void fill_white(Bitmap *bitmap, Int x, Int y, Bool *visited, Array<Point> *border)
{
    Array<Point> queue = create_array<Point>();
    Int head = 0;
    *queue.push() = {x, y};
    *(visited + bitmap->width * y + x) = true;

    while (queue.length - head > 0)
    {
        Int x = queue[head].x;
        Int y = queue[head].y;
        head++;

        for (Int i = 0; i < 4; i++)
        {
            Int new_x = x + inc_x[i];
            Int new_y = y + inc_y[i];

            if (new_x >= 0 && new_x < bitmap->width && new_y >= 0 && new_y < bitmap->height)
            {
                UInt32 pixel_color = get_pixel(bitmap, new_x, new_y);
                Bool pixel_visited = get_visited(bitmap, visited, new_x, new_y);

                if (!pixel_visited)
                {
                    if (pixel_color == 0xffffffff)
                    {
                        *(visited + bitmap->width * new_y + new_x) = true;

                        Point *queue_point = queue.push();
                        queue_point->x = new_x;
                        queue_point->y = new_y;

                        // set_pixel(bitmap, new_x, new_y, 0x00ff0000);
                    }
                    else
                    {
                        Point *border_point = border->push();
                        border_point->x = new_x;
                        border_point->y = new_y;
                    }
                }
            }
        }
    }

    destroy_array(queue);
}

void fill_shape(Bitmap *bitmap)
{
    Bool *visited = (Bool *)malloc(sizeof(Bool) * bitmap->width * bitmap->height);
    memset(visited, 0, sizeof(Bool) * bitmap->width * bitmap->height);

    Array<Point> border = create_array<Point>();

    for (Int x = 0; x < bitmap->width; x++)
    {
        {
            UInt32 pixel_color = get_pixel(bitmap, x, 0);
            Bool pixel_visited = get_visited(bitmap, visited, x, 0);
            if (!pixel_visited)
            {
                if (pixel_color == 0xffffffff)
                {
                    fill_white(bitmap, x, 0, visited, &border);
                }
                else
                {
                    *border.push() = {x, 0};
                }
            }
        }

        {
            UInt32 pixel_color = get_pixel(bitmap, x, bitmap->height - 1);
            Bool pixel_visited = get_visited(bitmap, visited, x, bitmap->height - 1);
            if (!pixel_visited)
            {
                if (pixel_color == 0xffffffff)
                {
                    fill_white(bitmap, x, bitmap->height - 1, visited, &border);
                }
                else
                {
                    *border.push() = {x, bitmap->height - 1};
                }
            }
        }
    }

    for (Int y = 0; y < bitmap->height; y++)
    {
        {
            UInt32 pixel_color = get_pixel(bitmap, 0, y);
            Bool pixel_visited = get_visited(bitmap, visited, 0, y);
            if (!pixel_visited)
            {
                if (pixel_color == 0xffffffff)
                {
                    fill_white(bitmap, 0, y, visited, &border);
                }
                else
                {
                    *border.push() = {0, y};
                }
            }
        }

        {
            UInt32 pixel_color = get_pixel(bitmap, bitmap->width - 1, y);
            Bool pixel_visited = get_visited(bitmap, visited, bitmap->width - 1, y);
            if (!pixel_visited)
            {
                if (pixel_color == 0xffffffff)
                {
                    fill_white(bitmap, bitmap->width - 1, y, visited, &border);
                }
                else
                {
                    *border.push() = {bitmap->width - 1, y};
                }
            }
        }
    }

    for (Int i = 0; i < border.length; i++)
    {
        Point *border_point = &border[i];
        Bool pixel_visited = get_visited(bitmap, visited, border_point->x, border_point->y);
        UInt32 pixel_color = get_pixel(bitmap, border_point->x, border_point->y);
        if (!pixel_visited && pixel_color == 0)
        {
            fill_black(bitmap, border_point->x, border_point->y, visited);
        }
    }

    free(visited);
    destroy_array(border);
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
