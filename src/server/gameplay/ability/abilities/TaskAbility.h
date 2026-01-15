//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_TASKABILITY_H
#define DEMO0_SERVER_TASKABILITY_H

#include "../AbilityBase.h"
#include "../task/AbilityTask.h"
#include <memory>

namespace ability
{
    class TaskAbility : public AbilityBase
    {
    public:
        bool IsActive() const override { return _active; }
        Phase CurrentPhase() const override { return _phase; }

        void Start(Context& ctx, const StartRequest& req) override
        {
            _active = true;
            _phase = Phase::Windup;
            _root = Build(ctx, req);
            if (_root) _root->Start(ctx);
        }

        void Tick(Context& ctx) override
        {
            if (!_active || !_root) return;

            // 同步字段（MVP：有活动就标记 slot/phase）
            if (ctx.outActiveSlot)  *ctx.outActiveSlot  = (uint8_t)BoundSlot();
            if (ctx.outActivePhase) *ctx.outActivePhase = (uint8_t)_phase;

            auto r = _root->Tick(ctx);
            if (r.status == TaskStatus::Running) return;

            _active = false;
            _phase = Phase::None;
        }

        void Cancel(Context& ctx) override
        {
            if (_root) _root->Cancel(ctx);
            _active = false;
            _phase = Phase::None;
        }

    protected:
        virtual std::unique_ptr<IAbilityTask> Build(Context& ctx, const StartRequest& req) = 0;

        bool _active = false;
        Phase _phase = Phase::None;
        std::unique_ptr<IAbilityTask> _root;
    };
}
#endif //DEMO0_SERVER_TASKABILITY_H
