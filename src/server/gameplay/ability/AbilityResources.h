//
// Created by tmsz on 26-1-4.
//

#ifndef DEMO0_SERVER_ABILITYRESOURCES_H
#define DEMO0_SERVER_ABILITYRESOURCES_H
#include <cstdint>

namespace ability
{
    // 资源位（bitmask）。你后续只要在这里扩展即可。
    enum ResourceFlags : uint32_t
    {
        Res_None     = 0,
        Res_Movement = 1u << 0, // 位移控制/移动注入（冲刺、击退等）
        Res_WeaponOp = 1u << 1, // 武器操作（开火/换弹/近战）
        Res_Ability  = 1u << 2, // 可选：能力动作“互斥槽”（想让某些能力互斥时用）
        // 未来：Res_Camera, Res_Mode, Res_Interact ...
    };

    // 你们 demo 规模下最多 32 个资源位足够
    constexpr int kMaxResources = 32;

    inline int ResourceBitToIndex(uint32_t bit)
    {
        // bit 必须是 2^n
        int idx = 0;
        while (((bit >> idx) & 1u) == 0u) ++idx;
        return idx;
    }
}


#endif //DEMO0_SERVER_ABILITYRESOURCES_H
