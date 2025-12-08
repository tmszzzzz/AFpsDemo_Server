//
// Created by tmsz on 25-12-3.
//
#include "KCC.h"
#include "../collision/CollisionWorld.h"

namespace kcc
{

    namespace
    {

        // 计算 OBB 的世界 AABB（假定 Obb 提供 center/halfExtents/rotationMatrix）
        // rotationMatrix 的三列为世界空间的局部 X/Y/Z 轴。
        Aabb ComputeObbWorldAabb(const Obb& obb)
        {
            // 假定 Obb 有：Vec3 center; Vec3 halfExtents; Vec3 axisX, axisY, axisZ;
            Vec3 axisX, axisY, axisZ;
            obb.rotation.ComputeObbAxesFromQuat(axisX, axisY, axisZ);

            Vec3 ax = axisX * obb.halfExtents.x;
            Vec3 ay = axisY * obb.halfExtents.y;
            Vec3 az = axisZ * obb.halfExtents.z;

            Vec3 ext{
                    std::fabs(ax.x) + std::fabs(ay.x) + std::fabs(az.x),
                    std::fabs(ax.y) + std::fabs(ay.y) + std::fabs(az.y),
                    std::fabs(ax.z) + std::fabs(ay.z) + std::fabs(az.z)
            };

            Aabb aabb;
            aabb.min = obb.center - ext;
            aabb.max = obb.center + ext;
            return aabb;
        }

        // 计算胶囊 swept AABB：考虑上下端球 + 运动 delta
        Aabb ComputeCapsuleSweptAabb(const Capsule& capsule, const Vec3& delta)
        {
            Vec3 top0    = capsule.topCenter();
            Vec3 bottom0 = capsule.bottomCenter();
            Vec3 top1    = top0 + delta;
            Vec3 bottom1 = bottom0 + delta;

            Vec3 minPt{
                    std::min(std::min(top0.x, bottom0.x), std::min(top1.x, bottom1.x)),
                    std::min(std::min(top0.y, bottom0.y), std::min(top1.y, bottom1.y)),
                    std::min(std::min(top0.z, bottom0.z), std::min(top1.z, bottom1.z))
            };

            Vec3 maxPt{
                    std::max(std::max(top0.x, bottom0.x), std::max(top1.x, bottom1.x)),
                    std::max(std::max(top0.y, bottom0.y), std::max(top1.y, bottom1.y)),
                    std::max(std::max(top0.z, bottom0.z), std::max(top1.z, bottom1.z))
            };

            Vec3 r{capsule.radius, capsule.radius, capsule.radius};

            Aabb aabb;
            aabb.min = minPt - r;
            aabb.max = maxPt + r;
            return aabb;
        }

        bool AabbOverlap(const Aabb& a, const Aabb& b)
        {
            if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
            if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
            if (a.max.z < b.min.z || a.min.z > b.max.z) return false;
            return true;
        }

        // 把世界坐标点变换到 OBB 局部空间：pLocal = R^T * (pWorld - center)
        Vec3 WorldToObbLocal(const Obb& obb, const Vec3& pWorld)
        {
            Vec3 axisX, axisY, axisZ;
            obb.rotation.ComputeObbAxesFromQuat(axisX, axisY, axisZ);

            Vec3 d = pWorld - obb.center;
            // 轴是单位向量，直接点乘
            return Vec3{
                    d.dot(axisX),
                    d.dot(axisY),
                    d.dot(axisZ)
            };
        }

        // 从 OBB 局部空间点变回世界：pWorld = center + axisX * x + axisY * y + axisZ * z
        Vec3 ObbLocalToWorld(const Obb& obb, const Vec3& pLocal)
        {
            Vec3 axisX, axisY, axisZ;
            obb.rotation.ComputeObbAxesFromQuat(axisX, axisY, axisZ);

            return obb.center
                   + axisX * pLocal.x
                   + axisY * pLocal.y
                   + axisZ * pLocal.z;
        }

        // 局部空间中，点到 AABB 的最近点
        Vec3 ClosestPointOnBoxLocal(const Vec3& p, const Vec3& halfExtents)
        {
            Vec3 q = p;
            q.x = std::max(-halfExtents.x, std::min(halfExtents.x, q.x));
            q.y = std::max(-halfExtents.y, std::min(halfExtents.y, q.y));
            q.z = std::max(-halfExtents.z, std::min(halfExtents.z, q.z));
            return q;
        }

        // 球 vs OBB 重叠测试
        bool SphereOverlapsObb(const Vec3& center, float radius, const Obb& obb)
        {
            Vec3 cLocal  = WorldToObbLocal(obb, center);
            Vec3 closest = ClosestPointOnBoxLocal(cLocal, obb.halfExtents);
            Vec3 diff    = cLocal - closest;
            float dist2  = diff.sqrMagnitude();
            return dist2 <= radius * radius;
        }

        // 球 vs OBB sweep：沿 delta 方向移动，返回是否命中及最早 t / normal
        bool SweepSphereObb(const Vec3& center,
                            float       radius,
                            const Vec3& delta,
                            const Obb&  obb,
                            float       sweepEps,
                            Hit&        outHit)
        {
            (void)sweepEps; // 目前暂未使用，可用于做 t 的微调

            // 1. 计算 OBB 局部轴
            Vec3 axisX, axisY, axisZ;
            obb.rotation.ComputeObbAxesFromQuat(axisX, axisY, axisZ);

            // 2. 转到 OBB 局部空间
            Vec3 c0Local = WorldToObbLocal(obb, center);

            // 初始重叠处理
            // 如果一开始球就已经与 OBB 相交，则直接返回 t = 0 的命中，
            // 法线从“盒子最近点 → 球心”的方向推导。
            if (SphereOverlapsObb(center, radius, obb))
            {
                const Vec3& he = obb.halfExtents;

                // 最近点（局部空间）：把 c0Local clamp 到盒子内
                Vec3 closestLocal{
                        std::max(-he.x, std::min(c0Local.x, he.x)),
                        std::max(-he.y, std::min(c0Local.y, he.y)),
                        std::max(-he.z, std::min(c0Local.z, he.z))
                };

                Vec3 nLocal = c0Local - closestLocal;
                float lenSq = nLocal.dot(nLocal);

                if (lenSq > 1e-6f)
                {
                    // 球心在盒子外或刚好在面附近：从最近点指向球心
                    float invLen = 1.0f / std::sqrt(lenSq);
                    nLocal = nLocal * invLen;
                }
                else
                {
                    // 球心在盒子内部：选择“到最近面的法线”
                    float dx = he.x - std::fabs(c0Local.x);
                    float dy = he.y - std::fabs(c0Local.y);
                    float dz = he.z - std::fabs(c0Local.z);

                    if (dx <= dy && dx <= dz)
                    {
                        nLocal = Vec3{ (c0Local.x >= 0.0f ? 1.0f : -1.0f), 0.0f, 0.0f };
                    }
                    else if (dy <= dz)
                    {
                        nLocal = Vec3{ 0.0f, (c0Local.y >= 0.0f ? 1.0f : -1.0f), 0.0f };
                    }
                    else
                    {
                        nLocal = Vec3{ 0.0f, 0.0f, (c0Local.z >= 0.0f ? 1.0f : -1.0f) };
                    }
                }

                // 转回世界空间法线
                Vec3 nWorld =
                        axisX * nLocal.x +
                        axisY * nLocal.y +
                        axisZ * nLocal.z;
                nWorld = nWorld.normalized();

                // 命中点：取当前球心沿法线反向退回一个半径（球面上的接触点）
                Vec3 hitWorld = center - nWorld * radius;

                outHit.hasHit = true;
                outHit.t      = 0.0f;
                outHit.point  = hitWorld;
                outHit.normal = nWorld;
                outHit.obb    = &obb;
                // outHit.walkable 由外层根据 obb.flags 填充

                return true;
            }
            //初始重叠处理结束

            Vec3 dLocal{
                    delta.dot(axisX),
                    delta.dot(axisY),
                    delta.dot(axisZ)
            };

            // 3. 将盒子膨胀 radius，转为 ray vs AABB 问题
            Vec3 ext = obb.halfExtents + Vec3{radius, radius, radius};

            float t0 = 0.0f;
            float t1 = 1.0f;
            int   hitAxis = -1;

            auto updateInterval = [&](float c, float dc, float e, int axis)->bool
            {
                if (std::fabs(dc) < 1e-8f)
                {
                    if (c < -e || c > e) return false;
                    return true;
                }

                float invD  = 1.0f / dc;
                float tNear = (-e - c) * invD;
                float tFar  = ( e - c) * invD;
                if (tNear > tFar) std::swap(tNear, tFar);

                if (tNear > t0)
                {
                    t0      = tNear;
                    hitAxis = axis;
                }
                if (tFar < t1) t1 = tFar;

                if (t0 > t1) return false;
                return true;
            };

            if (!updateInterval(c0Local.x, dLocal.x, ext.x, 0)) return false;
            if (!updateInterval(c0Local.y, dLocal.y, ext.y, 1)) return false;
            if (!updateInterval(c0Local.z, dLocal.z, ext.z, 2)) return false;

            if (t0 < 0.0f || t0 > 1.0f) return false;

            // 4. 命中点与局部法线
            Vec3 hitLocal = c0Local + dLocal * t0;

            Vec3 nLocal{0.0f, 0.0f, 0.0f};
            if (hitAxis == 0)
                nLocal.x = (dLocal.x > 0.0f) ? -1.0f : 1.0f;
            else if (hitAxis == 1)
                nLocal.y = (dLocal.y > 0.0f) ? -1.0f : 1.0f;
            else if (hitAxis == 2)
                nLocal.z = (dLocal.z > 0.0f) ? -1.0f : 1.0f;

            // 5. 转回世界空间
            Vec3 hitWorld = ObbLocalToWorld(obb, hitLocal);
            Vec3 nWorld   =
                    axisX * nLocal.x +
                    axisY * nLocal.y +
                    axisZ * nLocal.z;
            nWorld = nWorld.normalized();

            outHit.hasHit = true;
            outHit.t      = t0;
            outHit.point  = hitWorld;
            outHit.normal = nWorld;
            outHit.obb    = &obb;
            // outHit.walkable 由外层根据 obb.flags 填充

            return true;
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
            if (settings.maxPenetrationIterations == 0)
                return;

            // 用于穿透检测的“有效半径”（减去 skin 宽度）
            float sphereRadius = std::max(0.0f, capsule.radius - settings.skinWidth);
            if (sphereRadius <= 0.0f)
                return;

            const auto& obbs = world.boxes;

            for (std::uint32_t iter = 0; iter < settings.maxPenetrationIterations; ++iter)
            {
                bool anyPenetration = false;
                Vec3 totalCorrection = Vec3::zero();

                // 当前这一轮迭代下，近似胶囊的三个采样球心
                Vec3 sphereCenters[3] = {
                        capsule.topCenter(),
                        capsule.center,
                        capsule.bottomCenter()
                };

                // 遍历所有 OBB，累积“挤出向量”
                for (const Obb& obb : obbs)
                {
                    float bestDepthForObb = 0.0f;
                    Vec3  bestNormalForObb = Vec3::zero();

                    // 预先求出该 OBB 的局部轴，供后面 local->world 法线转换使用
                    Vec3 axisX, axisY, axisZ;
                    obb.rotation.ComputeObbAxesFromQuat(axisX, axisY, axisZ);

                    for (const Vec3& sphereCenter : sphereCenters)
                    {
                        // 没有重叠就跳过
                        if (!SphereOverlapsObb(sphereCenter, sphereRadius, obb))
                            continue;

                        // 计算 sphereCenter 在 OBB 局部空间的最近点与法线
                        Vec3 cLocal = WorldToObbLocal(obb, sphereCenter);
                        Vec3 closestLocal = ClosestPointOnBoxLocal(cLocal, obb.halfExtents);
                        Vec3 diffLocal = cLocal - closestLocal;
                        float distSq = diffLocal.sqrMagnitude();

                        Vec3  nWorld = Vec3::zero();
                        float penetrationDepth = 0.0f;

                        if (distSq > 1e-6f)
                        {
                            // 球心在盒子外部：从最近点指向球心方向为法线
                            float dist = std::sqrt(distSq);
                            Vec3 nLocal = diffLocal * (1.0f / dist);

                            nWorld =
                                    axisX * nLocal.x +
                                    axisY * nLocal.y +
                                    axisZ * nLocal.z;
                            nWorld = nWorld.normalized();

                            // 球与盒子的穿透深度（>0 则有真正的穿透）
                            penetrationDepth = sphereRadius - dist;
                        }
                        else
                        {
                            // 球心基本在盒子内部：选离最近面的方向作为法线
                            const Vec3& he = obb.halfExtents;
                            float dx = he.x - std::fabs(cLocal.x);
                            float dy = he.y - std::fabs(cLocal.y);
                            float dz = he.z - std::fabs(cLocal.z);

                            Vec3 nLocal{0.0f, 0.0f, 0.0f};
                            float minInside = dx;
                            int   axis = 0; // 0=x,1=y,2=z

                            if (dy < minInside) { minInside = dy; axis = 1; }
                            if (dz < minInside) { minInside = dz; axis = 2; }

                            if (axis == 0)
                                nLocal.x = (cLocal.x >= 0.0f ? 1.0f : -1.0f);
                            else if (axis == 1)
                                nLocal.y = (cLocal.y >= 0.0f ? 1.0f : -1.0f);
                            else
                                nLocal.z = (cLocal.z >= 0.0f ? 1.0f : -1.0f);

                            nWorld =
                                    axisX * nLocal.x +
                                    axisY * nLocal.y +
                                    axisZ * nLocal.z;
                            nWorld = nWorld.normalized();

                            // 球心在内部时，向最近面的外侧推：半径 + 距离面内的距离
                            penetrationDepth = sphereRadius + std::max(0.0f, minInside);
                        }

                        if (penetrationDepth <= 0.0f)
                            continue;

                        // 对同一个 OBB，取“最深的穿透方向”
                        if (penetrationDepth > bestDepthForObb)
                        {
                            bestDepthForObb   = penetrationDepth;
                            bestNormalForObb  = nWorld;
                        }
                    } // for sphereCenters

                    if (bestDepthForObb > 0.0f)
                    {
                        anyPenetration = true;
                        float pushDist = bestDepthForObb + settings.penetrationEpsilon;
                        totalCorrection += bestNormalForObb * pushDist;
                    }
                } // for obbs

                if (!anyPenetration)
                    break;

                // 如果累计修正已经很小，就认为基本稳定了
                if (totalCorrection.sqrMagnitude() <=
                    settings.penetrationEpsilon * settings.penetrationEpsilon)
                    break;

                // 应用这一轮的总挤出向量
                capsule.center += totalCorrection;
            }
        }

        // 沿给定位移 delta 对所有 OBB 做 sweep，找出最早一次碰撞。
        bool SweepAndFindFirstHit(const collision::CollisionWorld& world,
                                  const Capsule&        capsule,
                                  const Vec3&           delta,
                                  const Settings&       settings,
                                  Hit&                  outHit) {
            // 1. 先构造胶囊在本次位移下的 swept AABB，作为 broad-phase 过滤
            Aabb sweptCapsule = ComputeCapsuleSweptAabb(capsule, delta);

            bool foundAnyHit = false;
            float bestT = 1.0f;   // 当前已知的最早命中时间
            Hit bestHit;              // 对应的命中信息

            // 2. 遍历世界中的所有 OBB，做粗过滤 + 精确检测
            const std::vector<Obb> &obbs = world.boxes; // 假定 CollisionWorld 暴露一个 OBB 列表

            for (const Obb &obb: obbs) {
                // 2.1 broad-phase：如果本次胶囊 swept AABB 与此 OBB 的 AABB 根本不重叠，就不可能撞到
                Aabb obbAabb = ComputeObbWorldAabb(obb);
                if (!AabbOverlap(sweptCapsule, obbAabb))
                    continue;

                // 2.2 三球近似胶囊：上端/中部/下端三个球分别做 sweep，挑出最早命中
                Vec3 sphereCenters[3] = {
                        capsule.topCenter(),
                        capsule.center,
                        capsule.bottomCenter()
                };

                float localBestT = 1.0f;
                Hit localBestHit;
                bool localHasHit = false;

                float sphereRadius = std::max(0.0f, capsule.radius - settings.skinWidth);

                for (auto sphereCenter : sphereCenters) {
                    Hit h;
                    bool hit = SweepSphereObb(sphereCenter,
                                              sphereRadius,
                                              delta,
                                              obb,
                                              settings.sweepEpsilon,
                                              h);
                    if (!hit || !h.hasHit)
                        continue;

                    if (!localHasHit || h.t < localBestT) {
                        localHasHit = true;
                        localBestT = h.t;
                        localBestHit = h;
                    }
                }

                if (!localHasHit)
                    continue;

                // 2.3 与当前全局最佳比较，只保留最早的命中
                if (!foundAnyHit || localBestT < bestT) {
                    foundAnyHit = true;
                    bestT = localBestT;
                    bestHit = localBestHit;

                    // walkable 标记由 OBB 的 flags 推导，bit0 = Walkable
                    bestHit.walkable = (obb.flags & 0x1u) != 0;
                }
            }

            if (!foundAnyHit) {
                outHit.hasHit = false;
                return false;
            }

            outHit = bestHit;
            return true;
        }

        // 额外的向下 ground check / snap（主 loop 结束后调用）
        void DoGroundSnap(const collision::CollisionWorld& world,
                          Capsule&              capsule,
                          const Settings&       settings,
                          MoveResult&           inOutResult)
        {
            // 若不需要贴地或地面探测距离为 0，则直接返回
            if (settings.groundSnapDistance <= 0.0f)
                return;

            // 目前若已经明确有可靠的 onGround，可以选择跳过 snap
            // 这里保守起见：即使 onGround 为 true，也允许继续贴地以消除小缝隙

            Vec3 down{0.0f, -1.0f, 0.0f};
            float maxDist = settings.groundSnapDistance;

            // 从胶囊底部球心开始，沿着 -Y 方向做一小段 sweep
            Vec3 sphereStart = capsule.bottomCenter();
            float sphereRadius = std::max(0.0f, capsule.radius - settings.skinWidth);
            Vec3 delta = down * maxDist;

            bool  foundAny = false;
            float bestDist = maxDist;
            Hit   bestHit;

            for (const Obb& obb : world.boxes)
            {
                Hit h;
                bool hit = SweepSphereObb(sphereStart,
                                          sphereRadius,
                                          delta,
                                          obb,
                                          settings.sweepEpsilon,
                                          h);
                if (!hit || !h.hasHit)
                    continue;

                float hitDist = maxDist * h.t;
                if (hitDist < bestDist)
                {
                    bestDist = hitDist;
                    bestHit  = h;
                    bestHit.walkable = (obb.flags & 0x1u) != 0;
                    foundAny = true;
                }
            }

            if (!foundAny)
                return;

            // 必须是可行走表面，且法线坡度在可接受范围内
            if (!bestHit.walkable)
                return;

            if (!IsGroundNormal(bestHit.normal, settings.maxSlopeAngleDeg))
                return;

            // 将胶囊整体沿 -Y 推到命中点位置（减去 skin 宽度相当于球的半径调整）
            capsule.center += down * bestDist;

            inOutResult.onGround     = true;
            inOutResult.groundNormal = bestHit.normal;
            inOutResult.groundObject = bestHit.obb;
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
