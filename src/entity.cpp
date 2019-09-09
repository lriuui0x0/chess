#pragma once

#include "../lib/util.hpp"
#include "math.cpp"
#include "asset.cpp"

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

struct Entity
{
    Str name;
    Vec3 pos;
    Mat4 rotation;
    Mesh *mesh;
    CollisionBox collision_box;
};

Real check_collision(Ray *ray, CollisionBox *collision_box)
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
