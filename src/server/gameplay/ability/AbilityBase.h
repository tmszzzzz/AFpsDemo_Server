//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYBASE_H
#define DEMO0_SERVER_ABILITYBASE_H

#include "AbilitySlot.h"
#include "AbilityContext.h"
#include <cstdint>

namespace ability
{
    enum class Phase : uint8_t { None=0, Windup=1, Active=2, Recovery=3 };

    struct StartRequest
    {
        Slot slot = Slot::None;
    };

    class AbilityBase
    {
    public:
        virtual ~AbilityBase() = default;

        virtual const char* Name() const = 0;
        virtual Slot BoundSlot() const = 0;

        // 仲裁层调用：是否希望启动（读输入/内部状态）
        virtual bool WantsStart(const Context& ctx) const = 0;

        // 仲裁层调用：是否允许启动（冷却/资源/锁）
        virtual bool CanStart(const Context& ctx) const = 0;

        virtual void Start(Context& ctx, const StartRequest& req) = 0;
        virtual void Tick(Context& ctx) = 0;
        virtual void Cancel(Context& ctx) = 0;

        virtual bool IsActive() const = 0;
        virtual Phase CurrentPhase() const = 0;

        // 给 Arbiter 的元信息
        virtual uint32_t RequestedLocks() const { return 0; }
        virtual int Priority() const { return 0; }
        virtual bool CanBeInterrupted() const { return true; }
    };
}
#endif //DEMO0_SERVER_ABILITYBASE_H
