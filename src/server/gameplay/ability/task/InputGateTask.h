//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_INPUTGATETASK_H
#define DEMO0_SERVER_INPUTGATETASK_H
#pragma once

#include <memory>
#include "AbilityTask.h"
#include "../../../../utils/Utils.h"

namespace ability
{
    enum class GateMode : uint8_t { KeyDown, KeyHold, KeyUp };

    class InputGateTask final : public IAbilityTask
    {
    public:
        InputGateTask(KeyCode key, GateMode mode) : _key(key), _mode(mode) {}

        TaskTickResult Tick(Context& ctx) override
        {
            const auto& in = *ctx.input;
            bool ok = false;

            switch (_mode)
            {
                case GateMode::KeyDown:
                    ok = GetKeyDown(in.buttonsThisTick, in.buttonsDown, in.prevButtonsDown, _key);
                    break;
                case GateMode::KeyHold:
                    ok = GetKey(in.buttonsThisTick, in.buttonsDown, in.prevButtonsDown, _key);
                    break;
                case GateMode::KeyUp:
                    ok = GetKeyUp(in.buttonsThisTick, in.buttonsDown, in.prevButtonsDown, _key);
                    break;
                default: break;
            }

            return { ok ? TaskStatus::Succeeded : TaskStatus::Running };
        }

        const char* DebugName() const override { return "InputGate"; }

    private:
        KeyCode _key;
        GateMode _mode;
    };

    inline std::unique_ptr<IAbilityTask> MakeKeyDown(KeyCode k)
    {
        return std::make_unique<InputGateTask>(k, GateMode::KeyDown);
    }

    inline std::unique_ptr<IAbilityTask> MakeKeyHold(KeyCode k)
    {
        return std::make_unique<InputGateTask>(k, GateMode::KeyHold);
    }

    inline std::unique_ptr<IAbilityTask> MakeKeyUp(KeyCode k)
    {
        return std::make_unique<InputGateTask>(k, GateMode::KeyUp);
    }
}
#endif //DEMO0_SERVER_INPUTGATETASK_H
