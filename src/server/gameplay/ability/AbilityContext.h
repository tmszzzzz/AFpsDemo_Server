//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYCONTEXT_H
#define DEMO0_SERVER_ABILITYCONTEXT_H

#include <functional>
#include "../../../utils/Utils.h"
#include "AbilityFwd.h"

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

        // =========================
        // Arbiter 注入：当前正在被调用的 ability
        // =========================
        AbilityBase* self = nullptr;

        // =========================
        // 运行中资源申请/释放（用于 Task scope 或运行中临时占用）
        // - 返回 true：成功占用；false：被拒绝（优先级不够/owner不可抢占等）
        // - 约束：调用者默认就是 ctx.self
        // =========================
        std::function<bool(uint32_t resourceBit, int priority)> tryAcquireResource;
        std::function<void(uint32_t resourceBit)> releaseResource;
    };
}
#endif //DEMO0_SERVER_ABILITYCONTEXT_H
