#pragma once

#include "util.cpp"

struct Vec2
{
    Real4 x;
    Real4 y;
};

struct Vec3
{
    Real4 x;
    Real4 y;
    Real4 z;
};

struct Vec4
{
    Real4 x;
    Real4 y;
    Real4 z;
    Real4 w;
};

struct Mat2
{
    Vec2 x;
    Vec2 y;
};

struct Mat3
{
    Vec3 x;
    Vec3 y;
    Vec3 z;
};

struct Mat4
{
    Vec4 x;
    Vec4 y;
    Vec4 z;
    Vec4 w;
};
