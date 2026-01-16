//
// Created by tmsz on 26-1-16.
//

#ifndef DEMO0_SERVER_PROJECTILESYSTEM_H
#define DEMO0_SERVER_PROJECTILESYSTEM_H

#include <cstdint>
#include <functional>
#include <vector>
#include "../../../utils/Utils.h"

namespace projectile
{
    enum class ProjectileKind : uint8_t
    {
        Hitscan,
        Linear,
    };

    struct SpawnDesc
    {
        ProjectileKind kind = ProjectileKind::Linear;
        Vec3 origin = Vec3::zero();
        Vec3 direction = Vec3{0, 0, 1};
        float speed = 30.0f;
        float maxDistance = 100.0f;
        float lifeTime = 3.0f;
        float radius = 0.0f;
        uint32_t ownerPlayerId = 0;
    };

    struct HitResult
    {
        bool hit = false;
        float t = 0.0f;
        Vec3 point;
        Vec3 normal;
        uint32_t targetId = 0;
        bool isActor = false;
    };

    struct CollisionQuery
    {
        std::function<bool(const Vec3& origin,
                           const Vec3& dir,
                           float maxDist,
                           HitResult& outHit)> raycastWorld;

        std::function<bool(uint32_t ownerId,
                           const Vec3& origin,
                           const Vec3& dir,
                           float maxDist,
                           HitResult& outHit)> raycastActor;
    };

    enum class EventType : uint8_t
    {
        HitWorld,
        HitActor,
        Expired,
    };

    struct ProjectileEvent
    {
        EventType type = EventType::Expired;
        uint32_t projectileId = 0;
        uint32_t ownerPlayerId = 0;
        uint32_t targetId = 0;
        Vec3 point;
        Vec3 normal;
    };

    class ProjectileSystem
    {
    public:
        uint32_t Spawn(const SpawnDesc& desc);

        void Tick(float dt, const CollisionQuery& query);
        void CollectEvents(std::vector<ProjectileEvent>& out);

        bool FireHitscan(const SpawnDesc& desc, const CollisionQuery& query);

    private:
        struct Projectile
        {
            uint32_t id = 0;
            uint32_t ownerPlayerId = 0;
            Vec3 position;
            Vec3 velocity;
            float remainingLife = 0.0f;
            float remainingDistance = 0.0f;
            float radius = 0.0f;
        };

        uint32_t _nextId = 1;
        std::vector<Projectile> _projectiles;
        std::vector<ProjectileEvent> _events;

        void emitHitEvent(const Projectile& p, const HitResult& hit);
        void emitExpired(const Projectile& p);
    };
}

#endif //DEMO0_SERVER_PROJECTILESYSTEM_H
