//
// Created by tmsz on 26-1-15.
//

#ifndef DEMO0_SERVER_RELOADABILITY_H
#define DEMO0_SERVER_RELOADABILITY_H

#pragma once

#include "../task/TaskAbility.h"
#include "../AbilityResources.h"

namespace ability
{
    class ReloadAbility final : public TaskAbility
    {
    public:
        const char* Name() const override { return "Generic.Reload"; }
        Slot BoundSlot() const override { return Slot::Reload; }

        bool WantsStart(const Context& ctx) const override;
        bool CanStart(const Context& ctx) const override;

        uint32_t ClaimedResources() const override { return Res_WeaponOp; }

        int ResourcePriority(uint32_t resourceBit) const override
        {
            if (resourceBit == Res_WeaponOp) return 10;
            return 0;
        }

        void Start(Context& ctx, const StartRequest& req) override;
        void Cancel(Context& ctx) override;

    protected:
        std::unique_ptr<IAbilityTask> Build(Context& ctx, const StartRequest& req) override;

    private:
        float _reloadPointSec = 0.6f;
        float _postReloadHoldSec = 0.4f;
    };
}

#endif //DEMO0_SERVER_RELOADABILITY_H
