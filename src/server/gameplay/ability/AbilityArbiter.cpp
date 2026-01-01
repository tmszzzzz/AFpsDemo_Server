//
// Created by tmsz on 26-1-1.
//
#include "AbilityArbiter.h"
#include "AbilityBase.h"
#include "AbilityContext.h"

namespace ability
{
    void Arbiter::startAbility(Context& ctx, AbilityBase* ab)
    {
        _active = ab;
        StartRequest req{};
        req.slot = ab->BoundSlot();
        ab->Start(ctx, req);
        rebuildLocks();
    }

    void Arbiter::cancelActive(Context& ctx)
    {
        if (!_active) return;
        _active->Cancel(ctx);
        _active = nullptr;
        rebuildLocks();
    }

    void Arbiter::rebuildLocks()
    {
        _locks = 0;
        if (_active && _active->IsActive())
            _locks |= _active->RequestedLocks();
    }

    void Arbiter::Tick(Context& ctx, AbilityBase* const* abilities, int count)
    {
        // 1) 先 tick 当前 active
        if (_active)
        {
            _active->Tick(ctx);
            if (!_active->IsActive())
                _active = nullptr;
        }

        // 2) 选择候选（最高优先级）
        AbilityBase* best = nullptr;
        for (int i = 0; i < count; ++i)
        {
            AbilityBase* ab = abilities[i];
            if (!ab) continue;
            if (ab->IsActive()) continue;
            if (!ab->WantsStart(ctx)) continue;
            if (!ab->CanStart(ctx)) continue;

            if (!best || ab->Priority() > best->Priority())
                best = ab;
        }

        // 3) 如果有候选：
        if (best)
        {
            if (!_active)
            {
                startAbility(ctx, best);
            }
            else if (_active->CanBeInterrupted() && best->Priority() > _active->Priority())
            {
                cancelActive(ctx);
                startAbility(ctx, best);
            }
        }

        // 4) 聚合 locks 写回（给上层/其他系统）
        rebuildLocks();
        if (ctx.locks) *ctx.locks |= _locks;
    }
}