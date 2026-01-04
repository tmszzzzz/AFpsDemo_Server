//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYBASE_H
#define DEMO0_SERVER_ABILITYBASE_H

#include "AbilitySlot.h"
#include "AbilityContext.h"
#include "AbilityResources.h"
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

        // [ADD] 我占用哪些资源（bitmask）
        virtual uint32_t ClaimedResources() const { return Res_None; }

        // [ADD] 针对某个资源 bit 的优先级（只对 ClaimedResources() 里包含的 bit 有意义）
        // 建议：未申请的资源返回 0（或 INT_MIN），但 Arbiter 会先检查 mask。
        virtual int ResourcePriority(uint32_t resourceBit) const { (void)resourceBit; return 0; }
    };
}
#endif //DEMO0_SERVER_ABILITYBASE_H
