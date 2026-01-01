//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYTASK_H
#define DEMO0_SERVER_ABILITYTASK_H
#pragma once

#include "TaskStatus.h"
#include "../AbilityContext.h"

namespace ability
{
    class IAbilityTask
    {
    public:
        virtual ~IAbilityTask() = default;

        virtual void Start(Context& ctx) {}
        virtual TaskTickResult Tick(Context& ctx) = 0;
        virtual void Cancel(Context& ctx) {}

        virtual const char* DebugName() const { return "Task"; }
    };
}
#endif //DEMO0_SERVER_ABILITYTASK_H
