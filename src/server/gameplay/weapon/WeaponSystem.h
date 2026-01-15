//
// Created by tmsz on 26-1-15.
//

#ifndef DEMO0_SERVER_WEAPONSYSTEM_H
#define DEMO0_SERVER_WEAPONSYSTEM_H

#include <functional>
#include <cstdint>
#include "WeaponState.h"
#include "../../../utils/Utils.h"

namespace proto { struct GameEvent; }

namespace weapon
{
    class WeaponSystem
    {
    public:
        WeaponSystem();
        explicit WeaponSystem(const WeaponConfig& config);

        void Tick(const ServerInputFrame& in,
                  float dt,
                  uint32_t serverTick,
                  uint32_t ownerPlayerId,
                  const std::function<void(const proto::GameEvent&)>& emitEvent);

        void TryFire(uint32_t serverTick,
                     uint32_t ownerPlayerId,
                     const std::function<void(const proto::GameEvent&)>& emitEvent);

        void BeginReload(uint32_t serverTick,
                         uint32_t ownerPlayerId,
                         const std::function<void(const proto::GameEvent&)>& emitEvent);

        void CompleteReload(uint32_t serverTick,
                            uint32_t ownerPlayerId,
                            const std::function<void(const proto::GameEvent&)>& emitEvent);

        void CancelReload();

        const WeaponState& State() const { return _state; }
        int MagSize() const { return _cfg.magSize; }

    private:
        WeaponConfig _cfg{};
        WeaponState  _state{};

        void tryFire(uint32_t serverTick,
                     uint32_t ownerPlayerId,
                     const std::function<void(const proto::GameEvent&)>& emitEvent);

        void startReload(uint32_t serverTick,
                         uint32_t ownerPlayerId,
                         const std::function<void(const proto::GameEvent&)>& emitEvent);

        void finishReload(uint32_t serverTick,
                          uint32_t ownerPlayerId,
                          const std::function<void(const proto::GameEvent&)>& emitEvent);
    };
}

#endif //DEMO0_SERVER_WEAPONSYSTEM_H
