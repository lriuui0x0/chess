#pragma once

#include "util.cpp"

union Vec2 {
    struct
    {
        Real x;
        Real y;
    };
    Real data[2];
};

union Vec3 {
    struct
    {
        Real x;
        Real y;
        Real z;
    };
    Real data[3];
};

union Vec4 {
    struct
    {
        Real x;
        Real y;
        Real z;
        Real w;
    };
    Real data[4];
};

union Mat2 {
    struct
    {
        Vec2 x;
        Vec2 y;
    };
    Vec2 columns[2];
};

union Mat3 {
    struct
    {
        Vec3 x;
        Vec3 y;
        Vec3 z;
    };
    Vec3 columns[3];
};

union Mat4 {
    struct
    {
        Vec4 x;
        Vec4 y;
        Vec4 z;
        Vec4 w;
    };
    Vec4 columns[4];
};

Mat4 get_identity_matrix()
{
    Mat4 result;
    result.columns[0] = {1.0f, 0.0f, 0.0f, 0.0f};
    result.columns[1] = {0.0f, 1.0f, 0.0f, 0.0f};
    result.columns[2] = {0.0f, 0.0f, 1.0f, 0.0f};
    result.columns[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return result;
}

Mat4 get_translate_matrix(Real x, Real y, Real z)
{
    Mat4 result;
    result.columns[0] = {1.0f, 0.0f, 0.0f, 0.0f};
    result.columns[1] = {0.0f, 1.0f, 0.0f, 0.0f};
    result.columns[2] = {0.0f, 0.0f, 1.0f, 0.0f};
    result.columns[3] = {x, y, z, 1.0f};
    return result;
}
