//
// Created by tmsz on 26-1-1.
//
// [MOD] AbilityArbiter.cpp（关键逻辑替换）

#include "AbilityArbiter.h"
#include "AbilityBase.h"
#include "AbilityContext.h"
#include "AbilityResources.h"

namespace ability
{
    void Arbiter::rebuildOwners(AbilityBase* const* abilities, int count)
    {
        _actives.clear();
        for (auto& p : _resourceOwner) p = nullptr;

        for (int i = 0; i < count; ++i)
        {
            AbilityBase* ab = abilities[i];
            if (!ab || !ab->IsActive()) continue;

            _actives.push_back(ab);

            const uint32_t mask = ab->ClaimedResources();
            for (uint32_t bit = 1u; bit != 0u; bit <<= 1u)
            {
                if ((mask & bit) == 0u) continue;
                const int idx = ResourceBitToIndex(bit);
                _resourceOwner[idx] = ab;
            }
        }
    }

    void Arbiter::cancelAbility(Context& ctx, AbilityBase* ab)
    {
        if (!ab || !ab->IsActive()) return;
        ab->Cancel(ctx);
    }

    void Arbiter::startAbility(Context& ctx, AbilityBase* ab)
    {
        if (!ab) return;
        StartRequest req{};
        req.slot = ab->BoundSlot();
        ab->Start(ctx, req);
    }

    bool Arbiter::tryStart(Context& ctx, AbilityBase* candidate)
    {
        if (!candidate) return false;
        if (candidate->IsActive()) return false;
        if (!candidate->WantsStart(ctx)) return false;
        if (!candidate->CanStart(ctx)) return false;

        const uint32_t want = candidate->ClaimedResources();
        if (want == Res_None)
        {
            startAbility(ctx, candidate);
            return true;
        }

        // 收集冲突 owner（去重）
        AbilityBase* conflicts[32] = {};
        int conflictCount = 0;

        for (uint32_t bit = 1u; bit != 0u; bit <<= 1u)
        {
            if ((want & bit) == 0u) continue;

            const int idx = ResourceBitToIndex(bit);
            AbilityBase* owner = _resourceOwner[idx];
            if (!owner || !owner->IsActive()) continue;

            bool exists = false;
            for (int i = 0; i < conflictCount; ++i)
                if (conflicts[i] == owner) { exists = true; break; }

            if (!exists) conflicts[conflictCount++] = owner;
        }

        if (conflictCount == 0)
        {
            startAbility(ctx, candidate);
            return true;
        }

        // 核心：逐资源比较优先级（按资源位比较，而不是按 ability 比较）
        for (uint32_t bit = 1u; bit != 0u; bit <<= 1u)
        {
            if ((want & bit) == 0u) continue;

            const int idx = ResourceBitToIndex(bit);
            AbilityBase* owner = _resourceOwner[idx];
            if (!owner || !owner->IsActive()) continue;

            const int pNew   = candidate->ResourcePriority(bit);
            const int pOwner = owner->ResourcePriority(bit);

            // 你描述的策略：候选必须严格更高，否则拒绝
            if (!(pNew > pOwner))
                return false;
        }

        // 能抢占：取消冲突者（去重列表）
        for (int i = 0; i < conflictCount; ++i)
            cancelAbility(ctx, conflicts[i]);

        startAbility(ctx, candidate);
        return true;
    }

    void Arbiter::Tick(Context& ctx, AbilityBase* const* abilities, int count)
    {
        // 1) tick 所有 active（不依赖 _actives 旧值，稳健）
        for (int i = 0; i < count; ++i)
        {
            AbilityBase* ab = abilities[i];
            if (ab && ab->IsActive())
                ab->Tick(ctx);
        }

        // 2) 重建 owner 表（资源释放/结束都在这里体现）
        rebuildOwners(abilities, count);

        // 3) 允许同 tick 启动多个：按“候选的最高资源优先级”降序尝试
        std::vector<bool> tried(count, false);

        auto candidateScore = [](AbilityBase* ab) -> int
        {
            if (!ab) return -2147483647;
            const uint32_t want = ab->ClaimedResources();
            int best = -2147483647;
            for (uint32_t bit = 1u; bit != 0u; bit <<= 1u)
            {
                if ((want & bit) == 0u) continue;
                const int p = ab->ResourcePriority(bit);
                if (p > best) best = p;
            }
            return best;
        };

        while (true)
        {
            int bestIdx = -1;
            AbilityBase* best = nullptr;
            int bestScore = -2147483647;

            for (int i = 0; i < count; ++i)
            {
                if (tried[i]) continue;

                AbilityBase* ab = abilities[i];
                if (!ab) { tried[i] = true; continue; }
                if (ab->IsActive()) { tried[i] = true; continue; }
                if (!ab->WantsStart(ctx)) { tried[i] = true; continue; }
                if (!ab->CanStart(ctx)) { tried[i] = true; continue; }

                const int s = candidateScore(ab);
                if (!best || s > bestScore)
                {
                    best = ab;
                    bestIdx = i;
                    bestScore = s;
                }
            }

            if (!best) break;
            tried[bestIdx] = true;

            const bool started = tryStart(ctx, best);
            if (started)
            {
                // 启动/抢占会改变 owner 表，重建后继续尝试其他候选
                rebuildOwners(abilities, count);
            }
        }
    }
}
