//
// Created by tmsz on 26-1-16.
//

#ifndef DEMO0_SERVER_MELEEABILITY_H
#define DEMO0_SERVER_MELEEABILITY_H

#pragma once

#include "TaskAbility.h"
#include "../AbilityResources.h"

namespace ability
{
    class MeleeAbility final : public TaskAbility
    {
    public:
        const char* Name() const override { return "Generic.Melee"; }
        Slot BoundSlot() const override { return Slot::Secondary; }

        bool WantsStart(const Context& ctx) const override;
        bool CanStart(const Context& ctx) const override;

        uint32_t ClaimedResources() const override { return Res_WeaponOp; }

        int ResourcePriority(uint32_t resourceBit) const override
        {
            if (resourceBit == Res_WeaponOp) return 100;
            return 0;
        }

    protected:
        std::unique_ptr<IAbilityTask> Build(Context& ctx, const StartRequest& req) override;

    private:
        float _hitPointSec = 0.15f;
        float _postHitHoldSec = 0.25f;
    };
}

#endif //DEMO0_SERVER_MELEEABILITY_H
