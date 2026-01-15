//
// Created by tmsz on 26-1-15.
//

#ifndef DEMO0_SERVER_WEAPONSTATE_H
#define DEMO0_SERVER_WEAPONSTATE_H

#include <cstdint>

namespace weapon
{
    struct WeaponConfig
    {
        int   magSize = 30;
        float fireIntervalSec = 0.1f;
        float reloadSec = 1.5f;
        bool  allowAutoReload = true;
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
