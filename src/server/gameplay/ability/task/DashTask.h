//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_DASHTASK_H
#define DEMO0_SERVER_DASHTASK_H

#include <memory>
#include "AbilityTask.h"

namespace ability
{
    class DashTask final : public IAbilityTask
    {
    public:
        DashTask(float durationSec, float speed) : _dur(durationSec), _speed(speed) {}

        void Start(Context& ctx) override
        {
            _fired = false;
            if (ctx.requestDash) ctx.requestDash(_dur, _speed);
            _fired = true;
        }

        TaskTickResult Tick(Context&) override
        {
            // Dash 的持续由 MovementSource 自己管理；这里立即结束即可
            return { TaskStatus::Succeeded };
        }

        const char* DebugName() const override { return "Dash"; }

    private:
        float _dur = 0.0f;
        float _speed = 0.0f;
        bool _fired = false;
    };

    inline std::unique_ptr<IAbilityTask> MakeDash(float durationSec, float speed)
    {
        return std::make_unique<DashTask>(durationSec, speed);
    }
}
#endif //DEMO0_SERVER_DASHTASK_H
