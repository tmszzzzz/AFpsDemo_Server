//
// Created by tmsz on 26-1-15.
//

#include "WeaponSystem.h"
#include "../../../protocol/Messages.h"
#include <algorithm>

namespace weapon
{
    WeaponSystem::WeaponSystem()
        : WeaponSystem(WeaponConfig{})
    {
    }

    WeaponSystem::WeaponSystem(const WeaponConfig& config)
        : _cfg(config)
    {
        _state.magAmmo = _cfg.magSize;
        _state.isReloading = false;
        _state.reloadRemain = 0.0f;
        _state.fireCooldown = 0.0f;
    }

    void WeaponSystem::Tick(const ServerInputFrame& in,
                            float dt,
                            uint32_t serverTick,
                            uint32_t ownerPlayerId,
                            const std::function<void(const proto::GameEvent&)>& emitEvent)
    {
        if (dt <= 0.0f)
            return;

        _state.fireCooldown = std::max(0.0f, _state.fireCooldown - dt);

        if (_state.isReloading)
        {
            _state.reloadRemain -= dt;
            if (_state.reloadRemain <= 0.0f)
                finishReload(serverTick, ownerPlayerId, emitEvent);
        }

        (void)in;
    }

    void WeaponSystem::TryFire(uint32_t serverTick,
                               uint32_t ownerPlayerId,
                               const std::function<void(const proto::GameEvent&)>& emitEvent,
                               const Vec3& origin,
                               const Vec3& direction)
    {
        tryFire(serverTick, ownerPlayerId, emitEvent, origin, direction);
    }

    void WeaponSystem::BeginReload(uint32_t serverTick,
                                   uint32_t ownerPlayerId,
                                   const std::function<void(const proto::GameEvent&)>& emitEvent)
    {
        startReload(serverTick, ownerPlayerId, emitEvent);
    }

    void WeaponSystem::CompleteReload(uint32_t serverTick,
                                      uint32_t ownerPlayerId,
                                      const std::function<void(const proto::GameEvent&)>& emitEvent)
    {
        finishReload(serverTick, ownerPlayerId, emitEvent);
    }

    void WeaponSystem::CancelReload()
    {
        _state.isReloading = false;
        _state.reloadRemain = 0.0f;
    }

    void WeaponSystem::tryFire(uint32_t serverTick,
                               uint32_t ownerPlayerId,
                               const std::function<void(const proto::GameEvent&)>& emitEvent,
                               const Vec3& origin,
                               const Vec3& direction)
    {
        if (_state.isReloading || _state.fireCooldown > 0.0f)
            return;

        if (_state.magAmmo <= 0)
            return;

        _state.magAmmo -= 1;
        _state.fireCooldown = _cfg.fireIntervalSec;

        if (emitEvent)
        {
            proto::GameEvent ev{};
            ev.type = proto::GameEventType::WeaponFired;
            ev.serverTick = serverTick;
            ev.casterPlayerId = ownerPlayerId;
            ev.targetId = 0;
            ev.u8Param0 = static_cast<uint8_t>(_state.magAmmo);
            ev.u8Param1 = 0;
            ev.u32Param0 = 0;
            ev.f32Param0 = 0.0f;
            ev.f32Param1 = 0.0f;
            emitEvent(ev);
        }

        projectile::SpawnDesc spawn{};
        spawn.kind = _cfg.projectileKind;
        spawn.origin = origin;
        spawn.direction = direction;
        spawn.speed = _cfg.projectileSpeed;
        spawn.maxDistance = _cfg.projectileMaxDistance;
        spawn.lifeTime = _cfg.projectileLifeTime;
        spawn.radius = _cfg.projectileRadius;
        spawn.ownerPlayerId = ownerPlayerId;
        _pendingSpawns.push_back(spawn);
    }

    void WeaponSystem::CollectPendingSpawns(std::vector<projectile::SpawnDesc>& out)
    {
        out.insert(out.end(), _pendingSpawns.begin(), _pendingSpawns.end());
        _pendingSpawns.clear();
    }

    void WeaponSystem::startReload(uint32_t serverTick,
                                   uint32_t ownerPlayerId,
                                   const std::function<void(const proto::GameEvent&)>& emitEvent)
    {
        if (_state.isReloading)
            return;
        if (_state.magAmmo >= _cfg.magSize)
            return;

        _state.isReloading = true;
        _state.reloadRemain = _cfg.reloadSec;

        if (emitEvent)
        {
            proto::GameEvent ev{};
            ev.type = proto::GameEventType::WeaponReloadStarted;
            ev.serverTick = serverTick;
            ev.casterPlayerId = ownerPlayerId;
            ev.targetId = 0;
            ev.u8Param0 = static_cast<uint8_t>(_state.magAmmo);
            ev.u8Param1 = 0;
            ev.u32Param0 = 0;
            ev.f32Param0 = _cfg.reloadSec;
            ev.f32Param1 = 0.0f;
            emitEvent(ev);
        }
    }

    void WeaponSystem::finishReload(uint32_t serverTick,
                                    uint32_t ownerPlayerId,
                                    const std::function<void(const proto::GameEvent&)>& emitEvent)
    {
        if (!_state.isReloading)
            return;

        _state.magAmmo = _cfg.magSize;

        _state.isReloading = false;
        _state.reloadRemain = 0.0f;

        if (emitEvent)
        {
            proto::GameEvent ev{};
            ev.type = proto::GameEventType::WeaponReloadFinished;
            ev.serverTick = serverTick;
            ev.casterPlayerId = ownerPlayerId;
            ev.targetId = 0;
            ev.u8Param0 = static_cast<uint8_t>(_state.magAmmo);
            ev.u8Param1 = 0;
            ev.u32Param0 = 0;
            ev.f32Param0 = 0.0f;
            ev.f32Param1 = 0.0f;
            emitEvent(ev);
        }
    }
}
