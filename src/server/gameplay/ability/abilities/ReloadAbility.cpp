//
// Created by tmsz on 26-1-15.
//

#include "ReloadAbility.h"
#include "../task/CallFuncTask.h"
#include "../task/SeqTask.h"
#include "../task/WaitTask.h"
#include "../../../utils/Utils.h"

namespace ability
{
    bool ReloadAbility::WantsStart(const Context& ctx) const
    {
        if (!ctx.input) return false;
        if (ctx.weapon.isReloading) return false;
        if (ctx.weapon.magAmmo >= ctx.weapon.magSize) return false;
        return GetKeyDown(*ctx.input, BUTTON_RELOAD);
    }

    bool ReloadAbility::CanStart(const Context& ctx) const
    {
        if (!ctx.ability) return false;
        if (!ctx.ability->beginReload || !ctx.ability->finishReload || !ctx.ability->cancelReload) return false;
        return true;
    }

    void ReloadAbility::Start(Context& ctx, const StartRequest& req)
    {
        if (ctx.ability && ctx.ability->beginReload)
            ctx.ability->beginReload(ctx.serverTick);
        TaskAbility::Start(ctx, req);
    }

    void ReloadAbility::Cancel(Context& ctx)
    {
        if (ctx.ability && ctx.ability->cancelReload)
            ctx.ability->cancelReload();
        TaskAbility::Cancel(ctx);
    }

    std::unique_ptr<IAbilityTask> ReloadAbility::Build(Context&, const StartRequest&)
    {
        return MakeSeqT(
            MakeWait(_reloadPointSec),
            MakeCall([](Context& ctx) {
                if (ctx.ability && ctx.ability->finishReload)
                    ctx.ability->finishReload(ctx.serverTick);
            }),
            MakeWait(_postReloadHoldSec)
        );
    }
}
