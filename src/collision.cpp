#pragma once

#include "../lib/util.hpp"
#include "math.cpp"

struct Ray
{
    Vec3 pos;
    Vec3 dir;
};

struct CollisionBox
{
    Vec3 center;
    Vec3 radius;
};

Real check_box_collision(Ray *ray, CollisionBox *collision_box)
{
    Real t_min = 0;
    Real t_max = 100000000;
    for (Int i = 0; i < 3; i++)
    {
        Real reciprocal_d = 1 / ray->dir[i];
        Real t1 = (collision_box->center[i] - collision_box->radius[i] - ray->pos[i]) * reciprocal_d;
        Real t2 = (collision_box->center[i] + collision_box->radius[i] - ray->pos[i]) * reciprocal_d;
        if (t1 > t2)
        {
            swap(&t1, &t2);
        }

        t_min = MAX(t_min, t1);
        t_max = MIN(t_max, t2);
        if (t_min > t_max)
        {
            return -1;
        }
    }

    return t_min;
}

Real check_convex_hull_collision(Ray *ray, Int collision_hull_vertex_count, Vec3 *collision_hull_vertex_data)
{
    Real t_min = 0;
    Real t_max = 100000000;
    for (Int vertex_i = 0; vertex_i < collision_hull_vertex_count; vertex_i += 3)
    {
        Vec3 a = collision_hull_vertex_data[vertex_i];
        Vec3 b = collision_hull_vertex_data[vertex_i + 1];
        Vec3 c = collision_hull_vertex_data[vertex_i + 2];
        Vec3 normal = normalize(cross(b - a, c - b));
        Real dist = dot(a, normal);

        Real denom = dot(normal, ray->dir);
        Real numer = dist - dot(normal, ray->pos);
        if (denom == 0)
        {
            return -1;
        }
        else
        {
            Real t = numer / denom;
            if (denom < 0)
            {
                if (t > t_min)
                {
                    t_min = t;
                }
            }
            else
            {
                if (t < t_max)
                {
                    t_max = t;
                }
            }

            if (t_min > t_max)
            {
                return -1;
            }
        }
    }
    return t_min;
}
