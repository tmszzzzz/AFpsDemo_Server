//
// Created by tmsz on 25-12-2.
//

#ifndef DEMO0_SERVER_KCC_H
#define DEMO0_SERVER_KCC_H

#pragma once

#include <cstdint>
#include "../collision/CollisionWorld.h"   // 你前面定义的 Obb/CollisionWorld
#include "../gameplay/movement/core/PlayerState.h"

// 可以单独开一个命名空间，避免和 movement 混在一起
namespace kcc
{
    /// 胶囊体定义：竖直对齐 Y 轴
    ///
    /// 约定：center 为“胶囊中轴线的中心点”，
    /// 半高 halfHeight 为中轴线中心到端点（球心）的距离。
    ///
    /// 实际几何：
    /// - 上端球心：center + (0, +halfHeight, 0)
    /// - 下端球心：center + (0, -halfHeight, 0)
    /// - 半径为 radius
    ///
    /// 这样就和 PlayerState.Position 作为“角色几何中心”很容易对应上。
    struct Capsule
    {
        Vec3  center;      // 世界空间
        float radius = 0.5f;
        float halfHeight = 0.9f;

        // 计算上/下端球心（后面几何函数会用到）
        Vec3 topCenter() const    { return Vec3{center.x, center.y + halfHeight, center.z}; }
        Vec3 bottomCenter() const { return Vec3{center.x, center.y - halfHeight, center.z}; }
    };

    /// 移动命中的一次碰撞信息（内部使用）
    struct Hit
    {
        bool  hasHit = false;

        float t = 1.0f;    // 沿 sweep 向量的归一化时间 [0, 1]
        Vec3  point;       // 碰撞点（世界空间）
        Vec3  normal;      // 碰撞法线（世界空间，单位向量）

        const Obb* obb = nullptr; // 命中的 OBB

        bool  walkable = false;   // 是否可站立（根据 obb->flags 推导）
    };

    /// KCC 行为调节参数
    struct Settings
    {
        // 主循环最多迭代次数（防止在狭缝里死循环）
        std::uint32_t maxIterations = 4;

        // 皮肤厚度（character controller 常用概念）：
        // 与碰撞面保持的最小距离，避免浮点误差导致卡死。
        float skinWidth = 0.02f;

        // 大位移分段：如果一次 desiredDelta 太大，可以拆分成多次。
        // 例如 maxStepDistance = 3.0f 时，位移长度 > 3 的会分 2+ 次 sweep。
        float maxStepDistance = 3.0f;

        // 最大可站立坡度角（度数）。例如 45 度以内视为 ground。
        float maxSlopeAngleDeg = 60.0f;

        // 贴地距离：角色在空中但脚底非常接近地面时，会尝试向下 snap。
        float groundSnapDistance = 0.2f;

        // 碰撞后 slide 的最小速度阈值（太小就认为停下）
        float minSlideSpeed = 0.01f;

        // 穿透修正相关
        std::uint32_t maxPenetrationIterations = 4;
        float         penetrationEpsilon       = 0.001f;

        // 数值稳定相关 epsilon
        float sweepEpsilon = 1e-4f;
    };

    /// 调用一次 KCC 移动的结果
    struct MoveResult
    {
        Vec3 appliedDisplacement = Vec3::zero(); // 实际应用的位移（pos_final - pos_original）

        bool onGround = false;                   // 是否着地
        Vec3 groundNormal = Vec3{0, 1, 0};       // 地面法线（如果 onGround）

        const Obb* groundObject = nullptr;       // 当前地面对应的碰撞体（可选）
    };

    /// KCC 主接口（胶囊版本）
    ///
    /// 输入：
    ///   - world:     事先加载好的 CollisionWorld（静态 OBB 集合）
    ///   - capsule:   角色当前胶囊状态（位置 + 几何参数）
    ///   - desiredDelta: 本帧“理想位移”，通常由 CharacterMotor 计算
    ///   - settings:  一些可调参数
    ///
    /// 输出：
    ///   - capsule.center 会被更新到最终位置
    ///   - 返回 MoveResult，包含实际位移 & ground 状态
    MoveResult MoveCapsule(const collision::CollisionWorld& world,
                           Capsule&              capsule,
                           const Vec3&           desiredDelta,
                           const Settings&       settings);

    /// 一个方便给 gameplay 用的薄封装：直接基于 PlayerState 操作。
    ///
    /// 约定：
    ///   - state.Position 视为胶囊 center
    ///   - 胶囊的 radius/halfHeight 由形参给出（不同英雄可以不同）
    ///   - state.IsGrounded 由结果回写
    ///
    inline MoveResult MovePlayer(const collision::CollisionWorld& world,
                                 movement::PlayerState& state,
                                 const Vec3&           desiredDelta,
                                 const Settings&       settings,
                                 float capsuleRadius,
                                 float capsuleHalfHeight)
    {
        Capsule capsule;
        capsule.center     = state.Position;
        capsule.radius     = capsuleRadius;
        capsule.halfHeight = capsuleHalfHeight;

        MoveResult result = MoveCapsule(world, capsule, desiredDelta, settings);

        state.Position   = capsule.center;
        state.IsGrounded = result.onGround;

        return result;
    }
}


#endif //DEMO0_SERVER_KCC_H
