#pragma once

#include "../lib/util.hpp"
#include "math.cpp"
#include "asset.cpp"
#include "game.cpp"

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

struct Camera
{
    Vec3 pos;
    Quaternion rotation;
};

struct Entity
{
    Vec3 pos;
    Quaternion rotation;
    Vec3 scale;
    Mesh *mesh;
};

struct MoveAnimation
{
    Real t;
    Vec3 pos_from;
    Vec3 pos_to;
};

struct Piece : Entity
{
    Str name;
    CollisionBox collision_box;

    Bool is_moving;
    MoveAnimation move_animation;
};

struct Board : Entity
{
    CollisionBox collision_box[BOARD_SQUARE_COUNT];
};

Vec3 get_square_pos(Int row, Int column)
{
    ASSERT(row >= 0 && row < BOARD_ROW_COUNT);
    ASSERT(column >= 0 && column < BOARD_COLUMN_COUNT);
    return {(Real)(column * 100.0), 0, (Real)(row * 100.0)};
}
