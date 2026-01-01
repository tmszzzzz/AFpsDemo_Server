//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_SCOPEDLOCKSTASK_H
#define DEMO0_SERVER_SCOPEDLOCKSTASK_H
#pragma once

#include <memory>
#include "AbilityTask.h"

namespace ability
{
    class ScopedLocksTask final : public IAbilityTask
    {
    public:
        ScopedLocksTask(uint32_t mask, std::unique_ptr<IAbilityTask> child)
                : _mask(mask), _child(std::move(child)) {}

        void Start(Context& ctx) override
        {
            _applied = false;
            apply(ctx);
            if (_child) _child->Start(ctx);
        }

        TaskTickResult Tick(Context& ctx) override
        {
            if (!_child) { release(ctx); return { TaskStatus::Succeeded }; }

            auto r = _child->Tick(ctx);
            if (r.status == TaskStatus::Running) return r;

            // 子任务结束时释放锁
            release(ctx);
            return r;
        }

        void Cancel(Context& ctx) override
        {
            if (_child) _child->Cancel(ctx);
            release(ctx);
        }

        const char* DebugName() const override { return "ScopedLocks"; }

    private:
        uint32_t _mask = 0;
        std::unique_ptr<IAbilityTask> _child;
        bool _applied = false;

        void apply(Context& ctx)
        {
            if (_applied) return;
            if (ctx.locks) *ctx.locks |= _mask;
            _applied = true;
        }

        void release(Context& ctx)
        {
            if (!_applied) return;
            if (ctx.locks) *ctx.locks &= ~_mask;
            _applied = false;
        }
    };

    inline std::unique_ptr<IAbilityTask> MakeScopedLocks(uint32_t mask, std::unique_ptr<IAbilityTask> child)
    {
        return std::make_unique<ScopedLocksTask>(mask, std::move(child));
    }
}
#endif //DEMO0_SERVER_SCOPEDLOCKSTASK_H
