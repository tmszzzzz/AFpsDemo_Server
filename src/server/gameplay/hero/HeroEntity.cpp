//
// Created by tmsz on 26-1-2.
//

#include "HeroEntity.h"

#include "../ability/abilities/DashAbility.h"
#include "../ability/abilities/FireAbility.h"
#include "../ability/abilities/MeleeAbility.h"
#include "../ability/abilities/ReloadAbility.h"

namespace gameplay
{
    HeroEntity::HeroEntity(uint32_t playerId, std::unique_ptr<hero::HeroCore> core)
            : _playerId(playerId)
            , _core(std::move(core))
            , _abilities()
            , _weapon()
    {
        // MVP：默认装一个 DashAbility
        _abilities.Add(std::make_unique<ability::DashAbility>());
        _abilities.Add(std::make_unique<ability::FireAbility>());
        _abilities.Add(std::make_unique<ability::MeleeAbility>());
        _abilities.Add(std::make_unique<ability::ReloadAbility>());
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

        ability::AbilityCallbacks abilityCallbacks{};
        abilityCallbacks.requestDash = requestDash;
        abilityCallbacks.emitEvent = emitEvent;
        abilityCallbacks.tryFire = [this, emitEvent](uint32_t tick) {
            _weapon.TryFire(tick, _playerId, emitEvent);
        };
        abilityCallbacks.beginReload = [this, emitEvent](uint32_t tick) {
            _weapon.BeginReload(tick, _playerId, emitEvent);
        };
        abilityCallbacks.finishReload = [this, emitEvent](uint32_t tick) {
            _weapon.CompleteReload(tick, _playerId, emitEvent);
        };
        abilityCallbacks.cancelReload = [this]() {
            _weapon.CancelReload();
        };
        ctx.ability = &abilityCallbacks;

        _weapon.Tick(in, dt, serverTick, _playerId, emitEvent);

        const auto& ws = _weapon.State();
        ctx.weapon.magAmmo = ws.magAmmo;
        ctx.weapon.magSize = _weapon.MagSize();
        ctx.weapon.isReloading = ws.isReloading;

        _abilities.Tick(ctx);
    }
}

