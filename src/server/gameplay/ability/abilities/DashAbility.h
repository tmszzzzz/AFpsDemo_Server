//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_DASHABILITY_H
#define DEMO0_SERVER_DASHABILITY_H


#pragma once

#include "TaskAbility.h"
#include "../AbilityResources.h"   // [ADD]

namespace ability
{
    class DashAbility final : public TaskAbility
    {
    public:
        const char* Name() const override { return "Generic.Dash"; }
        Slot BoundSlot() const override { return Slot::Skill2; }

        bool WantsStart(const Context& ctx) const override;
        bool CanStart(const Context& ctx) const override;

        // [DEL] RequestedLocks / Priority / CanBeInterrupted

        // [ADD] Dash 占用移动资源
        uint32_t ClaimedResources() const override { return Res_Movement; }

        // [ADD] 仅对 Res_Movement 给优先级
        int ResourcePriority(uint32_t resourceBit) const override
        {
            if (resourceBit == Res_Movement) return 50; // 你可自行调参：比普通移动/被击退更高/更低等
            return 0;
        }

    protected:
        std::unique_ptr<IAbilityTask> Build(Context& ctx, const StartRequest& req) override;

    private:
        float _dashDuration = 0.1f;
        float _dashSpeed = 200.0f;
    };
}



#endif //DEMO0_SERVER_DASHABILITY_H
