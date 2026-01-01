//
// Created by tmsz on 26-1-1.
//

#include "DashAbility.h"
#include "../task/ScopedLocksTask.h"
#include "../task/DashTask.h"
#include "../task/SeqTask.h"

namespace ability
{
    bool DashAbility::WantsStart(const Context& ctx) const
    {
        const auto& in = *ctx.input;
        return GetKeyDown(in.buttonsThisTick, in.buttonsDown, in.prevButtonsDown, BUTTON_SKILL_SHIFT);
    }

    bool DashAbility::CanStart(const Context& ctx) const
    {
        // 需要的话可以检查 ctx.locks 或其他状态
        return true;
    }

    std::unique_ptr<IAbilityTask> DashAbility::Build(Context&, const StartRequest&) {
        // 流程：ScopedLocks(Ability) -> Dash(duration,speed)
        return MakeScopedLocks(Lock_Ability,
                               MakeSeqT(
                                       MakeDash(_dashDuration, _dashSpeed)
                               ));
    }
}
