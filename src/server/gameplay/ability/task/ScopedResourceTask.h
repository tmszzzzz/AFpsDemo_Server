//
// Created by tmsz on 26-1-14.
//

#ifndef DEMO0_SERVER_SCOPEDRESOURCETASK_H
#define DEMO0_SERVER_SCOPEDRESOURCETASK_H

#include <memory>
#include "AbilityTask.h"
#include "../AbilityResources.h" // Res_xxx + ResourceBitToIndex(如果需要)

namespace ability
{
    enum class AcquireMode : uint8_t
    {
        TryFail, // 抢不到就 Failed
        Wait     // 抢不到就 Running（下一 tick 继续尝试）
    };

    class ScopedResourceTask final : public IAbilityTask
    {
    public:
        ScopedResourceTask(uint32_t bit, int prio, AcquireMode mode, std::unique_ptr<IAbilityTask> child)
            : _bit(bit), _prio(prio), _mode(mode), _child(std::move(child)) {}

        void Start(Context&) override
        {
            _acquired = false;
            _childStarted = false;
        }

        TaskTickResult Tick(Context& ctx) override
        {
            // 1) 先拿资源；拿不到则不进入 scope
            if (!_acquired)
            {
                if (!ctx.tryAcquireResource || !ctx.releaseResource)
                    return { TaskStatus::Failed };

                const bool ok = ctx.tryAcquireResource(_bit, _prio);
                if (!ok)
                    return { (_mode == AcquireMode::Wait) ? TaskStatus::Running : TaskStatus::Failed };

                _acquired = true;
            }

            // 2) 拿到资源后，才启动 child（确保“拿不到不产生副作用”）
            if (_child && !_childStarted)
            {
                _child->Start(ctx);
                _childStarted = true;
            }

            // 3) 无 child：scope 立即结束
            if (!_child)
            {
                release(ctx);
                return { TaskStatus::Succeeded };
            }

            // 4) Tick child；离开 scope 即释放资源
            auto r = _child->Tick(ctx);
            if (r.status == TaskStatus::Running)
                return r;

            release(ctx);
            return r;
        }

        void Cancel(Context& ctx) override
        {
            if (_child) _child->Cancel(ctx);
            release(ctx);
        }

        const char* DebugName() const override { return "ScopedResource"; }

    private:
        uint32_t _bit = 0;
        int _prio = 0;
        AcquireMode _mode = AcquireMode::TryFail;

        std::unique_ptr<IAbilityTask> _child;
        bool _acquired = false;
        bool _childStarted = false;

        void release(Context& ctx)
        {
            if (!_acquired) return;

            // 释放 owner 表占用（你们当前 releaseRuntime 正是干这个）
            ctx.releaseResource(_bit);

            _acquired = false;
        }
    };

    inline std::unique_ptr<IAbilityTask> MakeScopedResource(uint32_t bit, int prio, AcquireMode mode, std::unique_ptr<IAbilityTask> child)
    {
        return std::make_unique<ScopedResourceTask>(bit, prio, mode, std::move(child));
    }
}

#endif //DEMO0_SERVER_SCOPEDRESOURCETASK_H
