//
// Created by tmsz on 26-1-15.
//

#ifndef DEMO0_SERVER_CALLFUNCTASK_H
#define DEMO0_SERVER_CALLFUNCTASK_H
#pragma once

#include "AbilityTask.h"
#include <functional>

namespace ability
{
    class CallFuncTask final : public IAbilityTask
    {
    public:
        explicit CallFuncTask(std::function<void(Context&)> fn)
            : _fn(std::move(fn))
        {
        }

        void Start(Context& ctx) override
        {
            if (_fn) _fn(ctx);
        }

        TaskTickResult Tick(Context&) override
        {
            return { TaskStatus::Succeeded };
        }

        const char* DebugName() const override { return "CallFunc"; }

    private:
        std::function<void(Context&)> _fn;
    };

    inline std::unique_ptr<IAbilityTask> MakeCall(std::function<void(Context&)> fn)
    {
        return std::make_unique<CallFuncTask>(std::move(fn));
    }
}

#endif //DEMO0_SERVER_CALLFUNCTASK_H
