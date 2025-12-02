//
// Created by tmsz on 25-12-2.
//

#ifndef DEMO0_SERVER_UTILS_H
#define DEMO0_SERVER_UTILS_H

#include <cmath>
#include <cstdint>

struct Vec3
{
    float x{0}, y{0}, z{0};

    Vec3() = default;
    Vec3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    Vec3 operator+(const Vec3& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    Vec3 operator-(const Vec3& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    Vec3 operator*(float s) const         { return {x * s, y * s, z * s}; }
    Vec3& operator+=(const Vec3& rhs)     { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    Vec3& operator-=(const Vec3& rhs)     { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
    Vec3& operator*=(float s)             { x *= s; y *= s; z *= s; return *this; }

    float dot(const Vec3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }

    float sqrMagnitude() const { return x * x + y * y + z * z; }
    float magnitude() const    { return std::sqrt(sqrMagnitude()); }

    Vec3 normalized() const
    {
        float m = magnitude();
        if (m <= 1e-6f) return Vec3{};
        return Vec3{x / m, y / m, z / m};
    }

    static Vec3 zero() { return Vec3{0, 0, 0}; }
};

struct Quat
{
    float x, y, z, w;
};

// 与 Unity 导出的 BoxCollider 对应的 OBB
struct Obb
{
    Vec3  center;      // 世界空间中心
    Quat  rotation{};    // 世界空间旋转(四元数 x,y,z,w)
    Vec3  halfExtents; // 世界空间半尺寸
    uint32_t flags{};    // bit0 = Walkable
};

#endif //DEMO0_SERVER_UTILS_H
