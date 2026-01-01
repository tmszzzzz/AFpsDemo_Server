//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_WAITTASK_H
#define DEMO0_SERVER_WAITTASK_H

#include <memory>
#include "AbilityTask.h"

namespace ability
{
    class WaitTask final : public IAbilityTask
    {
    public:
        explicit WaitTask(float sec) : _sec(sec) {}

        void Start(Context&) override { _remain = _sec; }

        TaskTickResult Tick(Context& ctx) override
        {
            _remain -= ctx.dt;
            return { (_remain <= 0.0f) ? TaskStatus::Succeeded : TaskStatus::Running };
        }

        const char* DebugName() const override { return "Wait"; }

    private:
        float _sec = 0.0f;
        float _remain = 0.0f;
    };

    inline std::unique_ptr<IAbilityTask> MakeWait(float sec)
    {
        return std::make_unique<WaitTask>(sec);
    }
}
#endif //DEMO0_SERVER_WAITTASK_H
