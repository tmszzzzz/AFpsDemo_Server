//
// Created by tmsz on 25-12-3.
//
#include "Kcc.h"
#include "../collision/CollisionWorld.h"

namespace kcc
{

    namespace
    {
        // 根据 desiredDelta 拆分步长，避免单次位移过大导致穿墙。
        float ComputeStepFraction(const Vec3& desiredDelta, float maxStepDistance)
        {
            float len = desiredDelta.magnitude();
            if (len <= 1e-6f) return 0.0f;
            if (len <= maxStepDistance) return 1.0f;
            return maxStepDistance / len; // 0~1，表示本次只走这么多比例
        }

        // 穿透修正：如果当前 capsule 已经与若干 OBB 重叠，尝试挤出。
        void ResolveInitialPenetration(const collision::CollisionWorld& world,
                                       Capsule&              capsule,
                                       const Settings&       settings)
        {
            // TODO:
            // 1. 找到所有和 capsule 重叠的 OBB
            // 2. 对每个求一个推离向量
            // 3. 合成一个小的修正位移，迭代几次
        }

        // 沿给定位移 delta 对所有 OBB 做 sweep，找出最早一次碰撞。
        bool SweepAndFindFirstHit(const collision::CollisionWorld& world,
                                  const Capsule&        capsule,
                                  const Vec3&           delta,
                                  const Settings&       settings,
                                  Hit&                  outHit)
        {
            // TODO:
            // 1. broad-phase：用 swept AABB 过滤 OBB
            // 2. 对每个候选 OBB 做 capsule vs OBB 的 sweep，求 t / normal
            // 3. 选 t 最小的一个，写入 outHit
            outHit.hasHit = false;
            return false;
        }

        // 根据碰撞法线对剩余位移做 slide（投影到切线平面）
        Vec3 ComputeSlideVector(const Vec3& remainingDelta, const Vec3& normal)
        {
            // slideDelta = remainingDelta - normal * dot(remainingDelta, normal)
            float nDot = remainingDelta.dot(normal);
            return remainingDelta - normal * nDot;
        }

        // 判断某个碰撞法线是否可作为“地面”
        bool IsGroundNormal(const Vec3& normal, float maxSlopeAngleDeg)
        {
            // 以 Y 轴为“上方向”，通过 cos(theta) = n·up 判断坡度
            Vec3 up{0.0f, 1.0f, 0.0f};
            float cosTheta = normal.dot(up); // normal 是单位向量
            if (cosTheta <= 0.0f) return false; // 背面或侧面

            float maxRad = maxSlopeAngleDeg * (3.14159265f / 180.0f);
            float minCos = std::cos(maxRad);
            return cosTheta >= minCos;
        }

        // 额外的向下 ground check / snap（主 loop 结束后调用）
        void DoGroundSnap(const collision::CollisionWorld& world,
                          Capsule&              capsule,
                          const Settings&       settings,
                          MoveResult&           inOutResult)
        {
            // TODO:
            // 1. 从 capsule 当前位置往下做短距离 sweep
            // 2. 如果命中可站立平面且距离 < groundSnapDistance，
            //    则沿法线微调 capsule.center，并把 onGround/groundNormal 填好
        }
    } // namespace


    MoveResult MoveCapsule(const collision::CollisionWorld& world,
                           Capsule&              capsule,
                           const Vec3&           desiredDelta,
                           const Settings&       settings)
    {
        MoveResult result;

        // 原始位置
        Vec3 originalCenter = capsule.center;

        // 如果没有位移，就只做一次穿透修正和 ground 检测
        if (desiredDelta.sqrMagnitude() <= 1e-6f)
        {
            ResolveInitialPenetration(world, capsule, settings);

            // 这里不立即判断 ground，交由 DoGroundSnap 统一处理
            DoGroundSnap(world, capsule, settings, result);

            result.appliedDisplacement = capsule.center - originalCenter;
            return result;
        }

        // 第一步：初始穿透修正
        ResolveInitialPenetration(world, capsule, settings);

        // 将 desiredDelta 可能拆分成多段执行（防穿墙）
        Vec3 remainingDelta = desiredDelta;

        // 可以先用一个 overallGroundHit 记录这次移动过程中“最可靠”的地面
        bool anyGroundHit = false;
        Vec3 bestGroundNormal{0.0f, 1.0f, 0.0f};
        const Obb* bestGroundObb = nullptr;

        // 外层：根据 maxStepDistance 拆分位移
        float totalLen = desiredDelta.magnitude();
        float movedLen = 0.0f;

        while (movedLen + 1e-6f < totalLen)
        {
            float stepFrac = ComputeStepFraction(remainingDelta, settings.maxStepDistance);
            if (stepFrac <= 0.0f)
                break;

            Vec3 stepDelta = remainingDelta * stepFrac;

            // 对这一小步跑 slide + sweep 主循环
            Vec3 stepRemaining = stepDelta;

            for (std::uint32_t iter = 0; iter < settings.maxIterations; ++iter)
            {
                if (stepRemaining.sqrMagnitude() <= settings.sweepEpsilon * settings.sweepEpsilon)
                    break;

                Hit hit;
                bool hasHit = SweepAndFindFirstHit(world, capsule, stepRemaining, settings, hit);

                if (!hasHit || !hit.hasHit)
                {
                    // 完整走完 stepRemaining
                    capsule.center += stepRemaining;
                    break;
                }

                // 移动到碰撞点前一小步（减去 skinWidth）
                float moveT = hit.t;
                if (moveT < 0.0f) moveT = 0.0f;
                if (moveT > 1.0f) moveT = 1.0f;

                Vec3 moveVec = stepRemaining * moveT;
                capsule.center += moveVec;

                // 剩余位移
                Vec3 remainingAfterHit = stepRemaining * (1.0f - moveT);

                // 通过法线做 slide
                Vec3 slideDelta = ComputeSlideVector(remainingAfterHit, hit.normal);

                if (slideDelta.sqrMagnitude() <= settings.minSlideSpeed * settings.minSlideSpeed)
                {
                    // 太小就算停下
                    stepRemaining = Vec3::zero();
                    break;
                }

                stepRemaining = slideDelta;

                // ground 判定：记录到 anyGroundHit
                if (hit.walkable && IsGroundNormal(hit.normal, settings.maxSlopeAngleDeg))
                {
                    anyGroundHit   = true;
                    bestGroundNormal = hit.normal;
                    bestGroundObb  = hit.obb;
                }
            }

            float stepLen = stepDelta.magnitude();
            movedLen      += stepLen;
            remainingDelta = desiredDelta * ((totalLen - movedLen) / totalLen);
        }

        // 额外的 ground snap / 检查
        if (anyGroundHit)
        {
            result.onGround     = true;
            result.groundNormal = bestGroundNormal;
            result.groundObject = bestGroundObb;
        }

        DoGroundSnap(world, capsule, settings, result);

        result.appliedDisplacement = capsule.center - originalCenter;
        return result;
    }
}
