#pragma once

#include "char_string.cpp"
#include "bitmap.cpp"

#define MAX_LINE_COUNT (1000)

Int line_count;
Line lines[MAX_LINE_COUNT];

#define EPSILON (0.01)

Bool equal(Real x, Real y, Real precision = EPSILON)
{
    Bool result = ABS(x - y) < precision;
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

struct ActiveEdge
{
    Line *line;
    Real diff;
    Real x;
    Int mode;
    ActiveEdge *next;
};

#define MAX_ACTIVE_EDGE_COUNT (1000)
ActiveEdge active_edges[MAX_ACTIVE_EDGE_COUNT + 1];
Int active_edge_count;

ActiveEdge *allocate_active_edge()
{
    ASSERT(active_edge_count < MAX_ACTIVE_EDGE_COUNT);
    ActiveEdge *edge = &active_edges[active_edge_count++];
    edge->next = null;
    return edge;
}

Void insert_active_edge(ActiveEdge *active_edge_list, ActiveEdge *new_active_edge)
{
    new_active_edge->next = active_edge_list->next;
    active_edge_list->next = new_active_edge;
    ActiveEdge *active_edge = active_edge_list->next;
    ActiveEdge *parent = active_edge_list;
    while (active_edge->next && active_edge->x > active_edge->next->x)
    {
        ActiveEdge *next_edge = active_edge->next;
        parent->next = next_edge;
        active_edge->next = next_edge->next;
        next_edge->next = active_edge;
        parent = next_edge;
    }
}

UInt32 get_pixel_value(Real value)
{
    UInt32 result = value * 255;
    result = result | result << 8 | result << 16 | 0xff000000;
    return result;
}

UInt32 *get_pixel_address(Bitmap *bitmap, Int x, Int y)
{
    UInt32 *result = bitmap->data + y * bitmap->width + x;
    return result;
}

Bitmap scanline_fill(Array<Path> paths, Real min_x, Real max_x, Real min_y, Real max_y, Real scale)
{
    Bitmap bitmap = create_bitmap(ceil((max_x - min_x) * scale), ceil((max_y - min_y) * scale));

    line_count = 0;
    memset(lines, 0, sizeof(lines));
    for (Int path_i = 0; path_i < paths.count; path_i++)
    {
        Path *path = &paths[path_i];
        for (Int line_i = 0; line_i < path->lines.count; line_i++)
        {
            lines[line_count] = path->lines[line_i];
            Line *line = &lines[line_count];

            line->x0 = (line->x0 - min_x) * scale;
            line->x1 = (line->x1 - min_x) * scale;
            line->y0 = (line->y0 - min_y) * scale;
            line->y1 = (line->y1 - min_y) * scale;
            if (line->y0 > line->y1)
            {
                swap(&line->x0, &line->x1);
                swap(&line->y0, &line->y1);
            }

            if (!equal(line->y0, line->y1))
            {
                line_count++;
            }
        }
    }

    sort_lines(lines, 0, line_count - 1);

    Int current_line = 0;
    active_edge_count = 0;

    ActiveEdge *active_edge_list = &active_edges[MAX_ACTIVE_EDGE_COUNT];
    active_edge_list->next = null;
    for (Int y = 0; y < bitmap.height; y++)
    {
        Real sample_offset[5] = {0.1, 0.3, 0.5, 0.7, 0.9};
        for (Int sample_i = 0; sample_i < 5; sample_i++)
        {
            Real scan_y = y + sample_offset[sample_i];
            // Real scan_y = (y + sample_offset[sample_i]) / scale + min_y;

            ActiveEdge *parent = active_edge_list;
            for (ActiveEdge *active_edge = active_edge_list->next; active_edge; active_edge = active_edge->next)
            {
                if (active_edge->line->y1 < scan_y)
                {
                    parent->next = active_edge->next;
                    continue;
                }

                active_edge->x += active_edge->diff;
                parent = parent->next;
            }

            while (current_line < line_count)
            {
                Line *line = &lines[current_line];
                ASSERT(line->y0 != line->y1);
                if (line->y0 <= scan_y)
                {
                    if (line->y1 > scan_y)
                    {
                        ActiveEdge *active_edge = allocate_active_edge();
                        active_edge->line = line;
                        Real m = (line->x1 - line->x0) / (line->y1 - line->y0);
                        active_edge->x = line->x0 + m * (scan_y - line->y0);
                        active_edge->diff = m * 0.2;
                        insert_active_edge(active_edge_list, active_edge);
                    }
                    current_line++;
                }
                else
                {
                    break;
                }
            }

            Bool is_filling = false;
            Real start_x;
            ActiveEdge *active_edge = active_edge_list->next;
            while (active_edge)
            {
                Real current_x = active_edge->x;
                if (is_filling)
                {
                    Int first_x = floor(start_x);
                    Real first_value = (ceil(start_x) - start_x) * 0.2;
                    *get_pixel_address(&bitmap, first_x, y) += get_pixel_value(first_value);

                    Int first_full_x = ceil(start_x);
                    Int last_full_x = floor(current_x) - 1;
                    for (Int x = first_full_x; x <= last_full_x; x++)
                    {
                        *get_pixel_address(&bitmap, x, y) += get_pixel_value(0.2);
                    }

                    Int last_x = floor(current_x);
                    Real last_x_value = (current_x - floor(current_x)) * 0.2;
                    *get_pixel_address(&bitmap, last_x, y) += get_pixel_value(last_x_value);
                }

                if (active_edge->next && equal(active_edge->x, active_edge->next->x))
                {
                    Int mode = equal(scan_y, active_edge->line->y0, 0.2) ? 0 : equal(scan_y, active_edge->line->y1, 0.2) ? 1 : 1000;
                    mode += equal(scan_y, active_edge->next->line->y0, 0.2) ? 0 : equal(scan_y, active_edge->next->line->y1, 0.2) ? 1 : 1000;

                    active_edge = active_edge->next->next;
                    if (!(mode == 0 || mode == 2))
                    {
                        is_filling = !is_filling;
                    }
                }
                else
                {
                    active_edge = active_edge->next;
                    is_filling = !is_filling;
                }

                if (is_filling)
                {
                    start_x = current_x;
                }
            }
        }
    }
    return bitmap;
}
