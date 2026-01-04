//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYCONTEXT_H
#define DEMO0_SERVER_ABILITYCONTEXT_H

#include <cstdint>
#include <functional>
#include "../../../utils/Utils.h"

namespace proto { struct GameEvent; }

namespace ability
{
    struct Context
    {
        float dt = 0.0f;
        float serverTimeSec = 0.0f;
        uint32_t serverTick = 0;

        const ServerInputFrame* input = nullptr;
        uint32_t ownerPlayerId = 0;

        // 写回 snapshot 的同步字段（由 HeroEntity 传入指针）
        uint8_t*  outActiveSlot  = nullptr;
        uint8_t*  outActivePhase = nullptr;

        // 对接移动（由 HeroEntity 绑定到你们的 IMovementSource/KCC）
        std::function<void(float durationSec, float speed)> requestDash;

        // 对接事件广播（由 GameServer 绑定到 BroadcastGameEvent）
        std::function<void(const proto::GameEvent&)> emitEvent;
    };
}
#endif //DEMO0_SERVER_ABILITYCONTEXT_H
