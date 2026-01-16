//
// Created by tmsz on 26-1-16.
//

#include "FireAbility.h"
#include "../AbilityContext.h"

namespace ability
{
    bool FireAbility::WantsStart(const Context& ctx) const
    {
        return GetKey(*ctx.input, MOUSE_FIRE_PRI);
    }

    bool FireAbility::CanStart(const Context& ctx) const
    {
        if (!ctx.ability) return false;
        if (!ctx.input) return false;
        if (ctx.weapon.magAmmo == 0) return false;
        return ctx.ability->tryFire != nullptr;
    }

    void FireAbility::Start(Context&, const StartRequest&)
    {
        _active = true;
        _phase = Phase::Active;
    }

    void FireAbility::Tick(Context& ctx)
    {
        if (!_active) return;

        if (!ctx.input || !GetKey(*ctx.input, MOUSE_FIRE_PRI))
        {
            _active = false;
            _phase = Phase::None;
            return;
        }
        if (ctx.weapon.magAmmo == 0 && !ctx.weapon.isReloading)
        {
            _active = false;
            _phase = Phase::None;
            return;
        }

        if (ctx.outActiveSlot)  *ctx.outActiveSlot = static_cast<uint8_t>(BoundSlot());
        if (ctx.outActivePhase) *ctx.outActivePhase = static_cast<uint8_t>(_phase);

        if (ctx.ability && ctx.ability->tryFire)
            ctx.ability->tryFire(ctx.serverTick, ctx.fireOrigin, ctx.fireDirection);
    }

    void FireAbility::Cancel(Context&)
    {
        _active = false;
        _phase = Phase::None;
    }
}
