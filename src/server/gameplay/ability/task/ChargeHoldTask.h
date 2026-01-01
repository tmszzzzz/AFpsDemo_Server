//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_CHARGEHOLDTASK_H
#define DEMO0_SERVER_CHARGEHOLDTASK_H
#pragma once

#include <memory>
#include "AbilityTask.h"
#include "../../../../utils/Utils.h" // GetKey + KeyCode

namespace ability
{
    class ChargeHoldTask final : public IAbilityTask
    {
    public:
        ChargeHoldTask(KeyCode holdKey, float maxSec, float& outCharge01)
                : _key(holdKey), _max(maxSec), _out(outCharge01) {}

        void Start(Context&) override
        {
            _t = 0.0f;
            _out = 0.0f;
        }

        TaskTickResult Tick(Context& ctx) override
        {
            const auto& in = *ctx.input;
            const bool holding = GetKey(in.buttonsThisTick, in.buttonsDown, in.prevButtonsDown, _key);

            if (holding)
            {
                _t += ctx.dt;
                if (_t > _max) _t = _max;
                _out = (_max <= 0.0f) ? 1.0f : (_t / _max);
                return { TaskStatus::Running };
            }

            // 松开：成功结束
            return { TaskStatus::Succeeded };
        }

        const char* DebugName() const override { return "ChargeHold"; }

    private:
        KeyCode _key;
        float _max;
        float& _out;
        float _t = 0.0f;
    };

    inline std::unique_ptr<IAbilityTask> MakeChargeHold(KeyCode k, float maxSec, float& outCharge01)
    {
        return std::make_unique<ChargeHoldTask>(k, maxSec, outCharge01);
    }
}
#endif //DEMO0_SERVER_CHARGEHOLDTASK_H
