#pragma once

#include <cmath>
#include "util.cpp"

#define PI (3.141592653589793238462643383279502884l)

Real degree_to_radian(Real degree)
{
    return degree / 180 * PI;
}

union Vec2 {
    struct
    {
        Real x;
        Real y;
    };
    Real entries[2];
    Real &operator[](Int index);
};

Real &Vec2::operator[](Int index)
{
    assert(index >= 0 && index < 2);
    return this->entries[index];
}

union Vec3 {
    struct
    {
        Real x;
        Real y;
        Real z;
    };
    Real entries[3];
    Real &operator[](Int index);
};

Real &Vec3::operator[](Int index)
{
    assert(index >= 0 && index < 3);
    return this->entries[index];
}

Vec3 operator+(Vec3 u, Vec3 v)
{
    Vec3 result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    result.z = u.z + v.z;
    return result;
}

Vec3 operator-(Vec3 u, Vec3 v)
{
    Vec3 result;
    result.x = u.x - v.x;
    result.y = u.y - v.y;
    result.z = u.z - v.z;
    return result;
}

Vec3 operator-(Vec3 u)
{
    Vec3 result;
    result.x = -u.x;
    result.y = -u.y;
    result.z = -u.z;
    return result;
}

Vec3 operator*(Real s, Vec3 u)
{
    Vec3 result;
    result.x = s * u.x;
    result.y = s * u.y;
    result.z = s * u.z;
    return result;
}

Vec3 normalize(Vec3 u)
{
    Vec3 result;
    Real length = sqrtf(u.x * u.x + u.y * u.y + u.z * u.z);
    result.x = u.x / length;
    result.y = u.y / length;
    result.z = u.z / length;
    return result;
}

Real dot(Vec3 u, Vec3 v)
{
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

Vec3 cross(Vec3 u, Vec3 v)
{
    Vec3 result;
    result.x = u.y * v.z - u.z * v.y;
    result.y = u.z * v.x - u.x * v.z;
    result.z = u.x * v.y - u.y * v.x;
    return result;
}

union Vec4 {
    struct
    {
        Real x;
        Real y;
        Real z;
        Real w;
    };
    Real entries[4];
    Real &operator[](Int index);
};

Real &Vec4::operator[](Int index)
{
    assert(index >= 0 && index < 4);
    return this->entries[index];
}

Vec4 vec4(Vec3 u, Real w = 1)
{
    return {u.x, u.y, u.z, w};
}

Vec3 vec3(Vec4 u)
{
    return {u.x, u.y, u.z};
}

Vec4 perspective_divide(Vec4 u)
{
    return {u.x / u.w, u.y / u.w, u.z / u.w, 1};
}

Real dot(Vec4 u, Vec4 v)
{
    return u.x * v.x + u.y * v.y + u.z * v.z + u.w * v.w;
}

union Mat2 {
    struct
    {
        Vec2 x;
        Vec2 y;
    };
    Vec2 columns[2];
    Vec2 &operator[](Int index);
    Vec2 row(Int index);
};

Vec2 &Mat2::operator[](Int index)
{
    assert(index >= 0 && index < 2);
    return this->columns[index];
}

Vec2 Mat2::row(Int index)
{
    assert(index >= 0 && index < 2);
    return {this->columns[0][index], this->columns[1][index]};
}

union Mat3 {
    struct
    {
        Vec3 x;
        Vec3 y;
        Vec3 z;
    };
    Vec3 columns[3];
    Vec3 &operator[](Int index);
    Vec3 row(Int index);
};

Vec3 &Mat3::operator[](Int index)
{
    assert(index >= 0 && index < 3);
    return this->columns[index];
}

Vec3 Mat3::row(Int index)
{
    assert(index >= 0 && index < 3);
    return {this->columns[0][index], this->columns[1][index], this->columns[2][index]};
}

union Mat4 {
    struct
    {
        Vec4 x;
        Vec4 y;
        Vec4 z;
        Vec4 w;
    };
    Vec4 columns[4];
    Vec4 &operator[](Int index);
    Vec4 row(Int index);
};

Vec4 &Mat4::operator[](Int index)
{
    assert(index >= 0 && index < 4);
    return this->columns[index];
}

Vec4 Mat4::row(Int index)
{
    assert(index >= 0 && index < 4);
    return {this->columns[0][index], this->columns[1][index], this->columns[2][index], this->columns[3][index]};
}

Vec4 operator*(Mat4 transform, Vec4 u)
{
    Vec4 result;
    result.x = dot(transform.row(0), u);
    result.y = dot(transform.row(1), u);
    result.z = dot(transform.row(2), u);
    result.w = dot(transform.row(3), u);
    return result;
}

Vec3 operator*(Mat4 transform, Vec3 u)
{
    Vec3 result;
    result.x = dot(vec3(transform.row(0)), u);
    result.y = dot(vec3(transform.row(1)), u);
    result.z = dot(vec3(transform.row(2)), u);
    return result;
}

Mat4 operator*(Mat4 transform1, Mat4 transform2)
{
    Mat4 result;
    for (Int row = 0; row < 4; row++)
    {
        for (Int col = 0; col < 4; col++)
        {
            result[col][row] = dot(transform1.row(row), transform2[col]);
        }
    }
    return result;
}

Mat4 transpose(Mat4 transform)
{
    Mat4 result;
    result[0] = transform.row(0);
    result[1] = transform.row(1);
    result[2] = transform.row(2);
    result[3] = transform.row(3);
    return result;
}

Mat4 get_identity_matrix()
{
    Mat4 result;
    result[0] = {1, 0, 0, 0};
    result[1] = {0, 1, 0, 0};
    result[2] = {0, 0, 1, 0};
    result[3] = {0, 0, 0, 1};
    return result;
}

Mat4 get_translate_matrix(Real x, Real y, Real z)
{
    Mat4 result;
    result[0] = {1, 0, 0, 0};
    result[1] = {0, 1, 0, 0};
    result[2] = {0, 0, 1, 0};
    result[3] = {x, y, z, 1};
    return result;
}

Mat4 get_rotation_matrix_x(Real angle)
{
    Mat4 result;
    result[0] = {1, 0, 0, 0};
    result[1] = {0, cosf(angle), sinf(angle), 0};
    result[2] = {0, -sinf(angle), cosf(angle), 0};
    result[3] = {0, 0, 1};
    return result;
}

Mat4 get_rotation_matrix_y(Real angle)
{
    Mat4 result;
    result[0] = {cosf(angle), 0, -sinf(angle), 0};
    result[1] = {0, 1, 0, 0};
    result[2] = {sinf(angle), 0, cosf(angle), 0};
    result[3] = {0, 0, 0, 1};
    return result;
}

Mat4 get_rotation_matrix_z(Real angle)
{
    Mat4 result;
    result[0] = {cosf(angle), sinf(angle), 0, 0};
    result[1] = {-sinf(angle), cosf(angle), 0, 0};
    result[2] = {0, 0, 1, 0};
    result[3] = {0, 0, 0, 1};
    return result;
}

Mat4 get_view_matrix(Vec3 pos, Vec3 dir, Vec3 up)
{
    Mat4 result;
    Vec3 pos_z = normalize(dir);
    Vec3 pos_x = normalize(cross(pos_z, up));
    Vec3 pos_y = cross(pos_z, pos_x);
    result[0] = {pos_x.x, pos_y.x, pos_z.x, 0};
    result[1] = {pos_x.y, pos_y.y, pos_z.y, 0};
    result[2] = {pos_x.z, pos_y.z, pos_z.z, 0};
    result[3] = {-dot(pos, pos_x), -dot(pos, pos_y), -dot(pos, pos_z), 1};
    return result;
}

Mat4 get_normal_view_matrix(Vec3 pos, Vec3 dir, Vec3 up)
{
    Mat4 result;
    Vec3 pos_z = normalize(dir);
    Vec3 pos_x = normalize(cross(up, pos_z));
    Vec3 pos_y = cross(pos_z, pos_x);
    result[0] = {pos_x.x, pos_y.x, pos_z.x, 0};
    result[1] = {pos_x.y, pos_y.y, pos_z.y, 0};
    result[2] = {pos_x.z, pos_y.z, pos_z.z, 0};
    result[3] = {0, 0, 0, 1};
    return result;
}

Mat4 get_perspective_matrix(Real view_angle, Real aspect_ratio, Real near, Real far)
{
    Mat4 result;
    Real c = 1 / tanf(view_angle / 2);
    result[0] = {c / aspect_ratio, 0, 0, 0};
    result[1] = {0, c, 0, 0};
    result[2] = {0, 0, far / (far - near), 1};
    result[3] = {0, 0, -far * near / (far - near), 0};
    return result;
}
