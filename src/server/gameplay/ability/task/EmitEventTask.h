//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_EMITEVENTTASK_H
#define DEMO0_SERVER_EMITEVENTTASK_H

#include <memory>
#include "AbilityTask.h"
#include "../../../../protocol/Messages.h"

namespace proto { struct GameEvent; }

namespace ability
{
    class EmitEventTask final : public IAbilityTask
    {
    public:
        explicit EmitEventTask(proto::GameEvent ev) : _ev(std::move(ev)) {}

        void Start(Context& ctx) override
        {
            if (ctx.emitEvent) ctx.emitEvent(_ev);
        }

        TaskTickResult Tick(Context&) override { return { TaskStatus::Succeeded }; }

        const char* DebugName() const override { return "EmitEvent"; }

    private:
        proto::GameEvent _ev;
    };

    inline std::unique_ptr<IAbilityTask> MakeEmitEvent(proto::GameEvent ev)
    {
        return std::make_unique<EmitEventTask>(std::move(ev));
    }
}
#endif //DEMO0_SERVER_EMITEVENTTASK_H
