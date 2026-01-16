//
// Created by tmsz on 26-1-16.
//

#include "ProjectileSystem.h"
#include <algorithm>

namespace projectile
{
    uint32_t ProjectileSystem::Spawn(const SpawnDesc& desc)
    {
        if (desc.kind == ProjectileKind::Hitscan)
            return 0;

        Projectile p{};
        p.id = _nextId++;
        p.ownerPlayerId = desc.ownerPlayerId;
        p.position = desc.origin;
        p.velocity = desc.direction.normalized() * desc.speed;
        p.remainingLife = desc.lifeTime;
        p.remainingDistance = desc.maxDistance;
        p.radius = desc.radius;

        _projectiles.push_back(p);
        return p.id;
    }

    uint32_t ProjectileSystem::FireHitscan(const SpawnDesc& desc, const CollisionQuery& query)
    {
        if (desc.kind != ProjectileKind::Hitscan)
            return 0;

        HitResult best{};
        bool found = false;

        if (query.raycastWorld)
        {
            HitResult hit{};
            if (query.raycastWorld(desc.origin, desc.direction, desc.maxDistance, hit))
            {
                best = hit;
                found = true;
            }
        }

        if (query.raycastActor)
        {
            HitResult hit{};
            if (query.raycastActor(desc.ownerPlayerId, desc.origin, desc.direction, desc.maxDistance, hit))
            {
                if (!found || hit.t < best.t)
                {
                    best = hit;
                    found = true;
                }
            }
        }

        const uint32_t id = _nextId++;
        if (!found)
            return id;

        Projectile p{};
        p.id = id;
        p.ownerPlayerId = desc.ownerPlayerId;
        emitHitEvent(p, best);
        return id;
    }

    void ProjectileSystem::Tick(float dt, const CollisionQuery& query)
    {
        if (dt <= 0.0f)
            return;

        for (size_t i = 0; i < _projectiles.size(); /* no ++ */)
        {
            Projectile& p = _projectiles[i];
            const float stepDist = p.velocity.magnitude() * dt;

            bool remove = false;
            bool hitSomething = false;
            HitResult best{};

            if (stepDist > 0.0f)
            {
                Vec3 dir = p.velocity.normalized();
                float maxDist = std::min(stepDist, p.remainingDistance);

                if (query.raycastWorld)
                {
                    HitResult hit{};
                    if (query.raycastWorld(p.position, dir, maxDist, hit))
                    {
                        best = hit;
                        hitSomething = true;
                    }
                }

                if (query.raycastActor)
                {
                    HitResult hit{};
                    if (query.raycastActor(p.ownerPlayerId, p.position, dir, maxDist, hit))
                    {
                        if (!hitSomething || hit.t < best.t)
                        {
                            best = hit;
                            hitSomething = true;
                        }
                    }
                }

                if (hitSomething)
                {
                    emitHitEvent(p, best);
                    remove = true;
                }
                else
                {
                    p.position += dir * maxDist;
                    p.remainingDistance -= maxDist;
                }
            }

            p.remainingLife -= dt;
            if (!remove && (p.remainingLife <= 0.0f || p.remainingDistance <= 0.0f))
            {
                emitExpired(p);
                remove = true;
            }

            if (remove)
            {
                _projectiles[i] = _projectiles.back();
                _projectiles.pop_back();
                continue;
            }

            ++i;
        }
    }

    void ProjectileSystem::CollectEvents(std::vector<ProjectileEvent>& out)
    {
        out.insert(out.end(), _events.begin(), _events.end());
        _events.clear();
    }

    void ProjectileSystem::emitHitEvent(const Projectile& p, const HitResult& hit)
    {
        ProjectileEvent ev{};
        ev.type = hit.isActor ? EventType::HitActor : EventType::HitWorld;
        ev.projectileId = p.id;
        ev.ownerPlayerId = p.ownerPlayerId;
        ev.targetId = hit.targetId;
        ev.point = hit.point;
        ev.normal = hit.normal;
        _events.push_back(ev);
    }

    void ProjectileSystem::emitExpired(const Projectile& p)
    {
        ProjectileEvent ev{};
        ev.type = EventType::Expired;
        ev.projectileId = p.id;
        ev.ownerPlayerId = p.ownerPlayerId;
        _events.push_back(ev);
    }
}
