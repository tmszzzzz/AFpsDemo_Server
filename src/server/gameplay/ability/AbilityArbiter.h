//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYARBITER_H
#define DEMO0_SERVER_ABILITYARBITER_H
#pragma once

#include <cstdint>

namespace ability
{
    class AbilityBase;
    struct Context;

    class Arbiter
    {
    public:
        void Tick(Context& ctx, AbilityBase* const* abilities, int count);

        uint32_t Locks() const { return _locks; }
        AbilityBase* Active() const { return _active; }

    private:
        uint32_t _locks = 0;
        AbilityBase* _active = nullptr;

        void startAbility(Context& ctx, AbilityBase* ab);
        void cancelActive(Context& ctx);
        void rebuildLocks();
    };
}
#endif //DEMO0_SERVER_ABILITYARBITER_H
