#pragma once

#include "../lib/util.hpp"
#include "math.cpp"
#include "asset.cpp"

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
