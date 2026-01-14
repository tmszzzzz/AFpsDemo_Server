//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYARBITER_H
#define DEMO0_SERVER_ABILITYARBITER_H
#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include "AbilityFwd.h"

namespace ability
{
    class Arbiter
    {
    public:
        void Tick(Context& ctx, AbilityBase* const* abilities, int count);

        const std::vector<AbilityBase*>& Actives() const { return _actives; }

    private:
        std::vector<AbilityBase*> _actives;

        // owner 台账（不再每 tick 重建）
        AbilityBase* _resourceOwner[32] = {};

        // 记录每个 active ability 登记过的 claimed mask
        std::unordered_map<AbilityBase*, uint32_t> _claimedMask;

        // ---- helpers ----
        bool tryStart(Context& ctx, AbilityBase* candidate);

        // [MOD] start/cancel 变成“带登记/释放”的版本
        void startAbility(Context& ctx, AbilityBase* ab);
        void cancelAbility(Context& ctx, AbilityBase* ab);

        // [ADD] 增量维护
        void registerAbility(AbilityBase* ab, uint32_t mask);
        void unregisterAbility(AbilityBase* ab);

        // [ADD] 运行中资源申请/释放（给 ctx.tryAcquireResource / releaseResource 用）
        bool acquireRuntime(Context& ctx, AbilityBase* requester, uint32_t bit, int pNew);
        void releaseRuntime(AbilityBase* requester, uint32_t bit);
    };
}

#endif //DEMO0_SERVER_ABILITYARBITER_H
