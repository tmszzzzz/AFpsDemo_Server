//
// Created by tmsz on 26-1-2.
//

#include "HeroEntity.h"

#include "../ability/abilities/DashAbility.h"

namespace gameplay
{
    HeroEntity::HeroEntity(uint32_t playerId, std::unique_ptr<hero::HeroCore> core)
            : _playerId(playerId)
            , _core(std::move(core))
            , _abilities()
    {
        // MVP：默认装一个 DashAbility
        _abilities.Add(std::make_unique<ability::DashAbility>());
    }

    void HeroEntity::TickGameplay(float serverTimeSec,
                                  float dt,
                                  uint32_t serverTick,
                                  const ServerInputFrame& in,
                                  const std::function<void(float durationSec, float speed)>& requestDash,
                                  const std::function<void(const proto::GameEvent&)>& emitEvent,
                                  uint8_t* outActiveSlot,
                                  uint8_t* outActivePhase)
    {
        ability::Context ctx{};
        ctx.dt = dt;
        ctx.serverTimeSec = serverTimeSec;
        ctx.serverTick = serverTick;

        ctx.input = &in;
        ctx.ownerPlayerId = _playerId;

        // snapshot 目前没用也可以传 null；需要调试再接上
        ctx.outActiveSlot = outActiveSlot;
        ctx.outActivePhase = outActivePhase;

        ctx.requestDash = requestDash;
        ctx.emitEvent = emitEvent;

        _abilities.Tick(ctx);
    }
}

