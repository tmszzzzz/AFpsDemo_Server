//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_SEQTASK_H
#define DEMO0_SERVER_SEQTASK_H
#pragma once

#include "AbilityTask.h"
#include <memory>
#include <vector>

namespace ability
{
    using TaskPtr = std::unique_ptr<IAbilityTask>;

    class SeqTask final : public IAbilityTask
    {
    public:
        explicit SeqTask(std::vector<TaskPtr> tasks) : _tasks(std::move(tasks)) {}

        void Start(Context& ctx) override
        {
            _i = 0;
            if (!_tasks.empty()) _tasks[0]->Start(ctx);
        }

        TaskTickResult Tick(Context& ctx) override
        {
            if (_i >= _tasks.size()) return { TaskStatus::Succeeded };

            auto r = _tasks[_i]->Tick(ctx);
            if (r.status == TaskStatus::Running) return r;
            if (r.status != TaskStatus::Succeeded) return r;

            _i++;
            if (_i < _tasks.size()) _tasks[_i]->Start(ctx);

            return (_i >= _tasks.size())
                   ? TaskTickResult{ TaskStatus::Succeeded }
                   : TaskTickResult{ TaskStatus::Running };
        }

        void Cancel(Context& ctx) override
        {
            if (_i < _tasks.size()) _tasks[_i]->Cancel(ctx);
        }

        const char* DebugName() const override { return "Seq"; }

    private:
        std::vector<TaskPtr> _tasks;
        size_t _i = 0;
    };

    inline TaskPtr MakeSeq(std::vector<TaskPtr> tasks)
    {
        return std::make_unique<SeqTask>(std::move(tasks));
    }

    template<class... Ts>
    TaskPtr MakeSeqT(Ts&&... ts)
    {
        std::vector<ability::TaskPtr> v;
        v.reserve(sizeof...(ts));
        (v.emplace_back(std::forward<Ts>(ts)), ...);
        return ability::MakeSeq(std::move(v));
    }

}
#endif //DEMO0_SERVER_SEQTASK_H
