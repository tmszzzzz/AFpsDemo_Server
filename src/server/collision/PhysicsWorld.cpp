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

        bool RaySphereIntersect(const Vec3& origin,
                                const Vec3& dir,
                                float maxDist,
                                const Vec3& center,
                                float radius,
                                float& outT,
                                Vec3& outNormal)
        {
            Vec3 oc = origin - center;
            float b = oc.dot(dir);
            float c = oc.dot(oc) - radius * radius;
            float h = b * b - c;
            if (h < 0.0f)
                return false;

            float sqrtH = std::sqrt(h);
            float t = -b - sqrtH;
            if (t < 0.0f)
                t = -b + sqrtH;
            if (t < 0.0f || t > maxDist)
                return false;

            Vec3 hitPoint = origin + dir * t;
            Vec3 n = hitPoint - center;
            outT = t;
            outNormal = (n.sqrMagnitude() > 1e-6f) ? n.normalized() : dir * -1.0f;
            return true;
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
            Vec3 ba = b - a;
            Vec3 oa = origin - a;

            float baba = ba.dot(ba);
            float bard = ba.dot(d);
            float baoa = ba.dot(oa);
            float rdoa = d.dot(oa);
            float oaoa = oa.dot(oa);

            float aCoeff = baba - bard * bard;
            float bCoeff = baba * rdoa - baoa * bard;
            float cCoeff = baba * oaoa - baoa * baoa - radius * radius * baba;

            float bestT = maxDist + 1.0f;
            Vec3 bestNormal = Vec3::zero();
            Vec3 bestPoint = Vec3::zero();
            bool found = false;

            const float eps = 1e-6f;

            if (std::fabs(aCoeff) > eps)
            {
                float h = bCoeff * bCoeff - aCoeff * cCoeff;
                if (h >= 0.0f)
                {
                    float t = (-bCoeff - std::sqrt(h)) / aCoeff;
                    if (t >= 0.0f && t <= maxDist)
                    {
                        float y = baoa + t * bard;
                        if (y > 0.0f && y < baba)
                        {
                            Vec3 point = origin + d * t;
                            Vec3 axisPoint = a + ba * (y / baba);
                            Vec3 n = point - axisPoint;
                            bestT = t;
                            bestPoint = point;
                            bestNormal = (n.sqrMagnitude() > 1e-6f) ? n.normalized() : d * -1.0f;
                            found = true;
                        }
                    }
                }
            }

            float capT = 0.0f;
            Vec3 capNormal = Vec3::zero();
            if (RaySphereIntersect(origin, d, maxDist, a, radius, capT, capNormal))
            {
                if (capT < bestT)
                {
                    bestT = capT;
                    bestPoint = origin + d * capT;
                    bestNormal = capNormal;
                    found = true;
                }
            }

            if (RaySphereIntersect(origin, d, maxDist, b, radius, capT, capNormal))
            {
                if (capT < bestT)
                {
                    bestT = capT;
                    bestPoint = origin + d * capT;
                    bestNormal = capNormal;
                    found = true;
                }
            }

            if (!found)
                return false;

            outHit.hit = true;
            outHit.t = bestT;
            outHit.point = bestPoint;
            outHit.normal = bestNormal;
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
