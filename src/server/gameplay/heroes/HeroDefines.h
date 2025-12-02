//
// Created by tmsz on 25-12-2.
//

#ifndef DEMO0_SERVER_HERODEFINES_H
#define DEMO0_SERVER_HERODEFINES_H

#include <cstdint>
#include "../movement/core/PlayerState.h"

namespace hero
{
    enum class HeroId : std::uint32_t
    {
        None    = 0,
        Generic = 1,
        // TODO: more heroes...
    };

    struct HeroConfig
    {
        float MaxHp = 100.0f;

        // CharacterMotor 所需参数
        float Gravity  = 20.0f;
        float MinPitch = -89.0f;
        float MaxPitch =  89.0f;

        float HorizontalAccelerationGround  = 80.0f;
        float HorizontalDecelerationGround  = 60.0f;
        float HorizontalAccelerationAir     = 30.0f;
        float HorizontalDecelerationAir     = 20.0f;
    };

    struct HeroState
    {
        HeroId                HeroId = HeroId::None;
        movement::PlayerState playerState;        // 位置、速度、朝向等

        float Hp    = 0.0f;
        float MaxHp = 0.0f;
    };
}

#endif //DEMO0_SERVER_HERODEFINES_H
