//
// Created by tmsz on 26-1-16.
//

#include "CollisionQueries.h"
#include <cmath>
#include <limits>

namespace collision
{
    namespace
    {
        Vec3 WorldToObbLocal(const Obb& obb, const Vec3& pWorld)
        {
            Vec3 axisX, axisY, axisZ;
            obb.rotation.ComputeObbAxesFromQuat(axisX, axisY, axisZ);
            Vec3 d = pWorld - obb.center;
            return Vec3{ d.dot(axisX), d.dot(axisY), d.dot(axisZ) };
        }

        Vec3 ObbLocalToWorld(const Obb& obb, const Vec3& pLocal)
        {
            Vec3 axisX, axisY, axisZ;
            obb.rotation.ComputeObbAxesFromQuat(axisX, axisY, axisZ);
            return obb.center
                   + axisX * pLocal.x
                   + axisY * pLocal.y
                   + axisZ * pLocal.z;
        }

        bool RaycastObb(const Obb& obb,
                        const Vec3& origin,
                        const Vec3& dir,
                        float maxDist,
                        RaycastHit& outHit)
        {
            Vec3 axisX, axisY, axisZ;
            obb.rotation.ComputeObbAxesFromQuat(axisX, axisY, axisZ);

            Vec3 o = WorldToObbLocal(obb, origin);
            Vec3 d{ dir.dot(axisX), dir.dot(axisY), dir.dot(axisZ) };

            float tMin = 0.0f;
            float tMax = maxDist;
            int hitAxis = -1;
            float hitSign = 1.0f;

            auto updateAxis = [&](float oC, float dC, float e, int axis) -> bool
            {
                if (std::fabs(dC) < 1e-6f)
                {
                    if (oC < -e || oC > e) return false;
                    return true;
                }

                float invD = 1.0f / dC;
                float tNear = (-e - oC) * invD;
                float tFar  = ( e - oC) * invD;
                if (tNear > tFar) std::swap(tNear, tFar);

                if (tNear > tMin)
                {
                    tMin = tNear;
                    hitAxis = axis;
                    hitSign = (dC > 0.0f) ? -1.0f : 1.0f;
                }
                if (tFar < tMax) tMax = tFar;
                return tMin <= tMax;
            };

            if (!updateAxis(o.x, d.x, obb.halfExtents.x, 0)) return false;
            if (!updateAxis(o.y, d.y, obb.halfExtents.y, 1)) return false;
            if (!updateAxis(o.z, d.z, obb.halfExtents.z, 2)) return false;

            if (tMin < 0.0f || tMin > maxDist) return false;

            Vec3 hitLocal = o + d * tMin;
            Vec3 nLocal{0.0f, 0.0f, 0.0f};
            if (hitAxis == 0) nLocal.x = hitSign;
            else if (hitAxis == 1) nLocal.y = hitSign;
            else if (hitAxis == 2) nLocal.z = hitSign;

            Vec3 hitWorld = ObbLocalToWorld(obb, hitLocal);
            Vec3 nWorld =
                axisX * nLocal.x +
                axisY * nLocal.y +
                axisZ * nLocal.z;
            nWorld = nWorld.normalized();

            outHit.hit = true;
            outHit.t = tMin;
            outHit.point = hitWorld;
            outHit.normal = nWorld;
            outHit.obb = &obb;
            return true;
        }
    }

    bool RaycastWorld(const CollisionWorld& world,
                      const Vec3& origin,
                      const Vec3& dir,
                      float maxDist,
                      RaycastHit& outHit)
    {
        float bestT = std::numeric_limits<float>::max();
        RaycastHit best{};

        for (const auto& obb : world.boxes)
        {
            RaycastHit hit{};
            if (!RaycastObb(obb, origin, dir, maxDist, hit))
                continue;

            if (hit.t < bestT)
            {
                bestT = hit.t;
                best = hit;
            }
        }

        if (bestT == std::numeric_limits<float>::max())
            return false;

        outHit = best;
        return true;
    }
}
