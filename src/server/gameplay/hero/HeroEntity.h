//
// Created by tmsz on 26-1-2.
//

#ifndef DEMO0_SERVER_HEROENTITY_H
#define DEMO0_SERVER_HEROENTITY_H


#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "../hero/HeroCore.h"
#include "../ability/AbilitySet.h"
#include "../ability/AbilityContext.h"


namespace gameplay
{
    class HeroEntity
    {
    public:
        HeroEntity(uint32_t playerId, std::unique_ptr<hero::HeroCore> core);

        uint32_t PlayerId() const { return _playerId; }

        hero::HeroCore& Core() { return *_core; }
        const hero::HeroCore& Core() const { return *_core; }

        // 每 tick：在 KCC 移动/朝向更新之后调用
        //
        // requestDash:
        //   - 由上层绑定到你们现有的 MovementSource/KCC 注入逻辑
        // emitEvent:
        //   - 由上层绑定到 BroadcastGameEvent(...)
        //
        // outActiveSlot/outActivePhase:
        //   - 目前 snapshot 没用可不传；后续要做 action/state 同步再接入。
        void TickGameplay(float serverTimeSec,
                          float dt,
                          uint32_t serverTick,
                          const ServerInputFrame& in,
                          const std::function<void(float durationSec, float speed)>& requestDash,
                          const std::function<void(const proto::GameEvent&)>& emitEvent,
                          uint8_t* outActiveSlot = nullptr,
                          uint8_t* outActivePhase = nullptr);

        uint32_t Locks() const { return _locks; }

        ability::AbilitySet& Abilities() { return _abilities; }
        const ability::AbilitySet& Abilities() const { return _abilities; }

    private:
        uint32_t _playerId = 0;
        std::unique_ptr<hero::HeroCore> _core;

        ability::AbilitySet _abilities;

        // 由武器/技能等系统在本 tick 内 OR 写入。必须每 tick 清零。
        uint32_t _locks = 0;
    };
}



#endif //DEMO0_SERVER_HEROENTITY_H
