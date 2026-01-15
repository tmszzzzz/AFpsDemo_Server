//
// Created by tmsz on 26-1-16.
//

#include "MeleeAbility.h"
#include "../task/CallFuncTask.h"
#include "../task/SeqTask.h"
#include "../task/WaitTask.h"
#include "../../../protocol/Messages.h"
#include "../../../utils/Utils.h"

namespace ability
{
    bool MeleeAbility::WantsStart(const Context& ctx) const
    {
        if (!ctx.input) return false;
        return GetKeyDown(*ctx.input, BUTTON_HIT_V);
    }

    bool MeleeAbility::CanStart(const Context& ctx) const
    {
        if (!ctx.ability) return false;
        return ctx.ability->emitEvent != nullptr;
    }

    std::unique_ptr<IAbilityTask> MeleeAbility::Build(Context&, const StartRequest&)
    {
        return MakeSeqT(
            MakeWait(_hitPointSec),
            MakeCall([](Context& ctx) {
                if (!ctx.ability || !ctx.ability->emitEvent) return;

                proto::GameEvent ev{};
                ev.type = proto::GameEventType::MeleeHit;
                ev.serverTick = ctx.serverTick;
                ev.casterPlayerId = ctx.ownerPlayerId;
                ev.targetId = 0;
                ev.u8Param0 = 0;
                ev.u8Param1 = 0;
                ev.u32Param0 = 0;
                ev.f32Param0 = 0.0f;
                ev.f32Param1 = 0.0f;
                ctx.ability->emitEvent(ev);
            }),
            MakeWait(_postHitHoldSec)
        );
    }
}
