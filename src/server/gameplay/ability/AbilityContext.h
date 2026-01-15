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
    struct WeaponSnapshot
    {
        int  magAmmo = 0;
        int  magSize = 0;
        bool isReloading = false;
    };

    struct AbilityCallbacks
    {
        // 对接移动注入（由 HeroEntity 绑定）
        std::function<void(float durationSec, float speed)> requestDash;

        // 对接事件广播（由 GameServer 绑定）
        std::function<void(const proto::GameEvent&)> emitEvent;

        // 武器换弹接口（由 HeroEntity 绑定）
        std::function<void(uint32_t serverTick)> beginReload;
        std::function<void(uint32_t serverTick)> finishReload;
        std::function<void()> cancelReload;
    };

    struct ResourceCallbacks
    {
        // 运行中资源申请/释放（由 Arbiter 注入）
        std::function<bool(uint32_t resourceBit, int priority)> tryAcquire;
        std::function<void(uint32_t resourceBit)> release;
    };

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

        AbilityCallbacks*  ability = nullptr;
        ResourceCallbacks* resources = nullptr;

        WeaponSnapshot weapon{};

        // =========================
        // Arbiter 注入：当前正在被调用的 ability
        // =========================
        AbilityBase* self = nullptr;

    };
}
#endif //DEMO0_SERVER_ABILITYCONTEXT_H
