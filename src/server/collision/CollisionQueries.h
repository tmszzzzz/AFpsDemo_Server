//
// Created by tmsz on 26-1-16.
//

#ifndef DEMO0_SERVER_COLLISIONQUERIES_H
#define DEMO0_SERVER_COLLISIONQUERIES_H

#include "CollisionWorld.h"

namespace collision
{
    struct RaycastHit
    {
        bool hit = false;
        float t = 0.0f;
        Vec3 point;
        Vec3 normal;
        const Obb* obb = nullptr;
    };

    bool RaycastWorld(const CollisionWorld& world,
                      const Vec3& origin,
                      const Vec3& dir,
                      float maxDist,
                      RaycastHit& outHit);
}

#endif //DEMO0_SERVER_COLLISIONQUERIES_H
