//
// Created by tmsz on 26-1-16.
//

#ifndef DEMO0_SERVER_FIREABILITY_H
#define DEMO0_SERVER_FIREABILITY_H

#pragma once

#include "../AbilityBase.h"
#include "../AbilityResources.h"

namespace ability
{
    class FireAbility final : public AbilityBase
    {
    public:
        const char* Name() const override { return "Generic.Fire"; }
        Slot BoundSlot() const override { return Slot::Primary; }

        bool WantsStart(const Context& ctx) const override;
        bool CanStart(const Context& ctx) const override;

        void Start(Context& ctx, const StartRequest& req) override;
        void Tick(Context& ctx) override;
        void Cancel(Context& ctx) override;

        bool IsActive() const override { return _active; }
        Phase CurrentPhase() const override { return _phase; }

        uint32_t ClaimedResources() const override { return Res_WeaponOp; }

        int ResourcePriority(uint32_t resourceBit) const override
        {
            if (resourceBit == Res_WeaponOp) return 20;
            return 0;
        }

    private:
        bool _active = false;
        Phase _phase = Phase::None;
    };
}

#endif //DEMO0_SERVER_FIREABILITY_H
