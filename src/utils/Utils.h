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
    void ComputeObbAxesFromQuat(Vec3& axisX, Vec3& axisY, Vec3& axisZ) const
    {
        // 标准四元数 -> 3x3 旋转矩阵，列向量为局部 X/Y/Z 轴
        float xx = x * x;
        float yy = y * y;
        float zz = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        // 注意：这里按“列向量”理解，每个 axis 是一列
        axisX = Vec3{
                1.0f - 2.0f * (yy + zz),
                2.0f * (xy + wz),
                2.0f * (xz - wy)
        };

        axisY = Vec3{
                2.0f * (xy - wz),
                1.0f - 2.0f * (xx + zz),
                2.0f * (yz + wx)
        };

        axisZ = Vec3{
                2.0f * (xz + wy),
                2.0f * (yz - wx),
                1.0f - 2.0f * (xx + yy)
        };
    }
};

// 与 Unity 导出的 BoxCollider 对应的 OBB
struct Obb
{
    Vec3  center;      // 世界空间中心
    Quat  rotation{};    // 世界空间旋转(四元数 x,y,z,w)
    Vec3  halfExtents; // 世界空间半尺寸
    uint32_t flags{};    // bit0 = Walkable
};


// 简单 AABB 结构（世界空间）
struct Aabb
{
    Vec3 min;
    Vec3 max;
};


// server端统一定义按键编码
typedef uint32_t KeyCode;
static constexpr KeyCode MOUSE_FIRE_PRI = 1u << 0;
static constexpr KeyCode MOUSE_FIRE_SEC = 1u << 1;
static constexpr KeyCode BUTTON_JUMP = 1u << 2;
static constexpr KeyCode BUTTON_ULTRA = 1u << 3;
static constexpr KeyCode BUTTON_SKILL_E = 1u << 4;
static constexpr KeyCode BUTTON_SKILL_SHIFT = 1u << 5;
static constexpr KeyCode BUTTON_SKILL_CTRL = 1u << 6;
static constexpr KeyCode BUTTON_HIT_V = 1u << 7;

// 20Hz 世界快照（0.05 秒一帧）
constexpr float SNAPSHOT_INTERVAL_SEC = 0.05f;

// 角度弧度转换
constexpr float DEG2RAD = 3.1415926535f / 180.0f;

#endif //DEMO0_SERVER_UTILS_H
