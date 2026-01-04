//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYARBITER_H
#define DEMO0_SERVER_ABILITYARBITER_H
#pragma once

#include <cstdint>
#include <vector>

namespace ability
{
    class AbilityBase;
    struct Context;

    class Arbiter
    {
    public:
        void Tick(Context& ctx, AbilityBase* const* abilities, int count);

        const std::vector<AbilityBase*>& Actives() const { return _actives; }

    private:
        std::vector<AbilityBase*> _actives;
        AbilityBase* _resourceOwner[32] = {};

        // ---- helpers ----
        void rebuildOwners(AbilityBase* const* abilities, int count);

        bool tryStart(Context& ctx, AbilityBase* candidate); // 尝试启动：处理抢占/拒绝
        void cancelAbility(Context& ctx, AbilityBase* ab);   // 取消某个 active
        void startAbility(Context& ctx, AbilityBase* ab);    // 启动并登记资源
    };
}
#endif //DEMO0_SERVER_ABILITYARBITER_H
