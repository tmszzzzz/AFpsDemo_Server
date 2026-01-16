//
// Created by tmsz on 26-1-16.
//

#ifndef DEMO0_SERVER_PHYSICSWORLD_H
#define DEMO0_SERVER_PHYSICSWORLD_H

#include <cstdint>
#include <vector>
#include "../../utils/Utils.h"
#include "CollisionWorld.h"
#include "CollisionQueries.h"

namespace collision
{
    class PhysicsWorld
    {
    public:
        void SetStaticWorld(const CollisionWorld* world) { _staticWorld = world; }

        void RegisterActorCapsule(uint32_t actorId, float radius, float halfHeight);
        void UnregisterActor(uint32_t actorId);
        void UpdateActorCapsule(uint32_t actorId, const Vec3& center);

        bool RaycastWorld(const Vec3& origin, const Vec3& dir, float maxDist, RaycastHit& outHit) const;
        bool RaycastActors(uint32_t ignoreId, const Vec3& origin, const Vec3& dir, float maxDist, RaycastHit& outHit) const;

    private:
        struct ActorCapsule
        {
            uint32_t id = 0;
            Vec3 center;
            float radius = 0.5f;
            float halfHeight = 0.9f;
        };

        const CollisionWorld* _staticWorld = nullptr;
        std::vector<ActorCapsule> _actors;
    };
}

#endif //DEMO0_SERVER_PHYSICSWORLD_H
