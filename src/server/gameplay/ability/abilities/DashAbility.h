//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_DASHABILITY_H
#define DEMO0_SERVER_DASHABILITY_H


#pragma once

#include "../task/TaskAbility.h"
#include "../AbilityLocks.h"

namespace ability
{
    class DashAbility final : public TaskAbility
    {
    public:
        const char* Name() const override { return "Generic.Dash"; }
        Slot BoundSlot() const override { return Slot::Skill2; }

        bool WantsStart(const Context& ctx) const override;
        bool CanStart(const Context& ctx) const override;

        uint32_t RequestedLocks() const override { return Lock_Ability; }
        int Priority() const override { return 5; }

    protected:
        std::unique_ptr<IAbilityTask> Build(Context& ctx, const StartRequest& req) override;

    private:
        float _dashDuration = 0.18f;
        float _dashSpeed = 18.0f;
    };
}


#endif //DEMO0_SERVER_DASHABILITY_H
