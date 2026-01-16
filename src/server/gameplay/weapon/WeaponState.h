//
// Created by tmsz on 26-1-15.
//

#ifndef DEMO0_SERVER_WEAPONSTATE_H
#define DEMO0_SERVER_WEAPONSTATE_H

#include <cstdint>
#include "../projectile/ProjectileSystem.h"

namespace weapon
{
    struct WeaponConfig
    {
        int   magSize = 30;
        float fireIntervalSec = 0.1f;
        float reloadSec = 1.5f;
        bool  allowAutoReload = true;

        projectile::ProjectileKind projectileKind = projectile::ProjectileKind::Hitscan;
        float projectileSpeed = 60.0f;
        float projectileMaxDistance = 100.0f;
        float projectileLifeTime = 3.0f;
        float projectileRadius = 0.0f;
    };

    struct WeaponState
    {
        int   magAmmo = 0;
        bool  isReloading = false;
        float reloadRemain = 0.0f;
        float fireCooldown = 0.0f;
    };
}

#endif //DEMO0_SERVER_WEAPONSTATE_H
