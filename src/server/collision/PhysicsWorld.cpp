//
// Created by tmsz on 26-1-16.
//

#include "PhysicsWorld.h"
#include "CollisionQueries.h"
#include <algorithm>

namespace collision
{
    namespace
    {
        float Clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }

        Vec3 ClosestPointOnSegment(const Vec3& a, const Vec3& b, const Vec3& p, float& outT)
        {
            Vec3 ab = b - a;
            float ab2 = ab.sqrMagnitude();
            if (ab2 <= 1e-6f)
            {
                outT = 0.0f;
                return a;
            }
            float t = (p - a).dot(ab) / ab2;
            t = Clamp01(t);
            outT = t;
            return a + ab * t;
        }

        bool RaycastCapsule(const Vec3& origin,
                            const Vec3& dir,
                            float maxDist,
                            const Vec3& a,
                            const Vec3& b,
                            float radius,
                            collision::RaycastHit& outHit)
        {
            Vec3 d = dir.normalized();
            Vec3 ab = b - a;

            // Find the closest approach between ray and segment.
            float tSeg = 0.0f;
            float tRay = 0.0f;

            // Approximate by sampling the closest point on segment to ray origin + projection.
            float proj = (a - origin).dot(d);
            Vec3 p = origin + d * std::max(0.0f, proj);
            Vec3 c = ClosestPointOnSegment(a, b, p, tSeg);

            Vec3 oc = c - origin;
            tRay = oc.dot(d);
            if (tRay < 0.0f)
                tRay = 0.0f;

            Vec3 closest = origin + d * tRay;
            float dist2 = (closest - c).sqrMagnitude();
            if (dist2 > radius * radius || tRay > maxDist)
                return false;

            outHit.hit = true;
            outHit.t = tRay;
            outHit.point = closest;
            Vec3 n = closest - c;
            outHit.normal = (n.sqrMagnitude() > 1e-6f) ? n.normalized() : d * -1.0f;
            return true;
        }
    }

    void PhysicsWorld::RegisterActorCapsule(uint32_t actorId, float radius, float halfHeight)
    {
        ActorCapsule c{};
        c.id = actorId;
        c.center = Vec3::zero();
        c.radius = radius;
        c.halfHeight = halfHeight;
        _actors.push_back(c);
    }

    void PhysicsWorld::UnregisterActor(uint32_t actorId)
    {
        for (size_t i = 0; i < _actors.size(); ++i)
        {
            if (_actors[i].id == actorId)
            {
                _actors[i] = _actors.back();
                _actors.pop_back();
                return;
            }
        }
    }

    void PhysicsWorld::UpdateActorCapsule(uint32_t actorId, const Vec3& center)
    {
        for (auto& actor : _actors)
        {
            if (actor.id == actorId)
            {
                actor.center = center;
                return;
            }
        }
    }

    bool PhysicsWorld::RaycastWorld(const Vec3& origin, const Vec3& dir, float maxDist, RaycastHit& outHit) const
    {
        if (!_staticWorld)
            return false;

        collision::RaycastHit hit{};
        if (!collision::RaycastWorld(*_staticWorld, origin, dir, maxDist, hit))
            return false;

        outHit.hit = true;
        outHit.t = hit.t;
        outHit.point = hit.point;
        outHit.normal = hit.normal;
        outHit.targetId = 0;
        return true;
    }

    bool PhysicsWorld::RaycastActors(uint32_t ignoreId, const Vec3& origin, const Vec3& dir, float maxDist, RaycastHit& outHit) const
    {
        float bestT = maxDist + 1.0f;
        bool found = false;

        for (const auto& actor : _actors)
        {
            if (actor.id == ignoreId)
                continue;

            Vec3 top = actor.center + Vec3{0.0f, actor.halfHeight, 0.0f};
            Vec3 bottom = actor.center - Vec3{0.0f, actor.halfHeight, 0.0f};

            collision::RaycastHit hit{};
            if (!RaycastCapsule(origin, dir, maxDist, bottom, top, actor.radius, hit))
                continue;

            if (hit.t < bestT)
            {
                bestT = hit.t;
                outHit = hit;
                outHit.targetId = actor.id;
                found = true;
            }
        }

        return found;
    }
}
