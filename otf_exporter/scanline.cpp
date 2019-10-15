#pragma once

#include "char_string.cpp"
#include "bitmap.cpp"

#define MAX_LINE_COUNT (1000)

Int line_count;
Line lines[MAX_LINE_COUNT];

Bool equal(Real x, Real y)
{
    Bool result = ABS(x - y) < 0.001;
    return result;
}

Void sort_lines(Line *lines, Int low, Int high)
{
    if (low >= high)
    {
        return;
    }

    Int left = low;
    Int pivot = rand() % (high - low + 1) + low;
    swap(&lines[high], &lines[pivot]);
    for (Int i = low; i < high; i++)
    {
        if (lines[i].y0 < lines[high].y0)
        {
            swap(&lines[i], &lines[left++]);
        }
    }
    swap(&lines[high], &lines[left]);

    sort_lines(lines, low, left - 1);
    sort_lines(lines, left + 1, high);
}

Bitmap scanline_fill(Array<Path> paths, Real min_x, Real max_x, Real min_y, Real max_y, Real scale)
{
    Bitmap bitmap;
    bitmap.width = ceil((max_x - min_x) * scale);
    bitmap.height = ceil((max_y - min_y) * scale);
    bitmap.data = (UInt32 *)malloc(sizeof(UInt32) * bitmap.width * bitmap.height);

    line_count = 0;
    for (Int path_i = 0; path_i < paths.count; path_i++)
    {
        Path *path = &paths[path_i];
        for (Int line_i = 0; line_i < path->lines.count; line_i++)
        {
            Line *line = &path->lines[line_i];
            if (!equal(line->y0, line->y1))
            {
                lines[line_count++] = path->lines[line_i];
                line = &lines[line_count - 1];

                if (line->y0 > line->y1)
                {
                    swap(&line->x0, &line->x1);
                    swap(&line->y0, &line->y1);
                }

                line->x0 = (line->x0 - min_x) * scale;
                line->x1 = (line->x1 - min_x) * scale;
                line->y0 = (line->y0 - min_y) * scale;
                line->y1 = (line->y1 - min_y) * scale;
            }
        }
    }

    sort_lines(lines, 0, line_count - 1);

    for (Int y = 0; y < bitmap.height; y++)
    {
        Real super_sample_offset[5] = {0.1, 0.3, 0.5, 0.7, 0.9};
        Real pixel_value = 0;
        for (Int i = 0; i < 5; i++)
        {
            Real scan_y = y + super_sample_offset[i];
        }
    }
}
