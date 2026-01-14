//
// Created by tmsz on 26-1-1.
//

#include "AbilityArbiter.h"
#include "AbilityBase.h"
#include "AbilityContext.h"
#include "AbilityResources.h"

namespace ability
{
    // -----------------------------
    // [ADD] 增量登记/释放
    // -----------------------------
    void Arbiter::registerAbility(AbilityBase* ab, uint32_t mask)
    {
        if (!ab) return;

        // 加入 actives（去重：理论上不会重复，但做一下防御）
        bool exists = false;
        for (auto* a : _actives) { if (a == ab) { exists = true; break; } }
        if (!exists) _actives.push_back(ab);

        // 记录 claimed mask（后续 end/cancel 用）
        _claimedMask[ab] = mask;

        // 登记 owner
        for (uint32_t bit = 1u; bit != 0u; bit <<= 1u)
        {
            if ((mask & bit) == 0u) continue;
            const int idx = ResourceBitToIndex(bit);
            _resourceOwner[idx] = ab;
        }
    }

    void Arbiter::unregisterAbility(AbilityBase* ab)
    {
        if (!ab) return;

        // 从 actives 移除
        for (size_t i = 0; i < _actives.size(); ++i)
        {
            if (_actives[i] == ab)
            {
                _actives[i] = _actives.back();
                _actives.pop_back();
                break;
            }
        }

        // 释放它登记过的资源位（用 _claimedMask，而不是 ab->ClaimedResources()）
        uint32_t mask = Res_None;
        auto it = _claimedMask.find(ab);
        if (it != _claimedMask.end())
        {
            mask = it->second;
            _claimedMask.erase(it);
        }

        for (uint32_t bit = 1u; bit != 0u; bit <<= 1u)
        {
            if ((mask & bit) == 0u) continue;
            const int idx = ResourceBitToIndex(bit);
            if (_resourceOwner[idx] == ab)
                _resourceOwner[idx] = nullptr;
        }
    }

    // -----------------------------
    // [ADD] 运行中资源申请/释放（给 Task scope 用）
    // 策略与你的 tryStart 一致：pNew 必须严格大于 pOwner 才能抢占
    // -----------------------------
    bool Arbiter::acquireRuntime(Context& ctx, AbilityBase* requester, uint32_t bit, int pNew)
    {
        if (!requester) return false;
        const int idx = ResourceBitToIndex(bit);
        AbilityBase* owner = _resourceOwner[idx];

        if (!owner || !owner->IsActive() || owner == requester)
        {
            _resourceOwner[idx] = requester;
            // 同时把该 bit 记入 requester 的 claimedMask（避免“运行中拿到但 end/cancel 不释放”）
            _claimedMask[requester] |= bit;
            return true;
        }

        const int pOwner = owner->ResourcePriority(bit);
        if (!(pNew > pOwner))
            return false;

        // 抢占：取消 owner（取消后会释放其 mask & owner 表）
        cancelAbility(ctx, owner);

        // owner 被取消后，本 bit 应为空；此时登记给 requester
        _resourceOwner[idx] = requester;
        _claimedMask[requester] |= bit;
        return true;
    }

    void Arbiter::releaseRuntime(AbilityBase* requester, uint32_t bit)
    {
        if (!requester) return;
        const int idx = ResourceBitToIndex(bit);
        if (_resourceOwner[idx] == requester)
            _resourceOwner[idx] = nullptr;

        // 注意：这里不从 _claimedMask[requester] 清除此 bit（因为它可能是“能力级长期资源”）
        // 如果你未来真的需要“可撤销的临时占用”，可以引入第二张 runtimeMask。
    }

    // -----------------------------
    // [MOD] cancel/start：带增量释放/登记
    // -----------------------------
    void Arbiter::cancelAbility(Context& ctx, AbilityBase* ab)
    {
        if (!ab || !ab->IsActive()) return;

        // Arbiter 调用 ability 时写 ctx.self，便于任务系统使用
        ctx.self = ab;
        ab->Cancel(ctx);
        ctx.self = nullptr;

        // Cancel 后一定要释放登记
        unregisterAbility(ab);
    }

    void Arbiter::startAbility(Context& ctx, AbilityBase* ab)
    {
        if (!ab) return;

        StartRequest req{};
        req.slot = ab->BoundSlot();

        ctx.self = ab;
        ab->Start(ctx, req);
        ctx.self = nullptr;

        // Start 后如果 active，则登记资源
        if (ab->IsActive())
        {
            const uint32_t mask = ab->ClaimedResources();
            registerAbility(ab, mask);
        }
    }

    // -----------------------------
    // [MOD] tryStart：逻辑基本不变，但不再依赖 rebuildOwners
    // -----------------------------
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
        // 0) 注入 runtime 资源接口（Task scope 可用）
        ctx.tryAcquireResource = [this, &ctx](uint32_t bit, int prio) -> bool
        {
            return this->acquireRuntime(ctx, ctx.self, bit, prio);
        };
        ctx.releaseResource = [this, &ctx](uint32_t bit)
        {
            this->releaseRuntime(ctx.self, bit);
        };

        // 1) Tick 所有 active（用 _actives；期间可能结束）
        //    结束后必须 unregister，以保持台账一致
        for (size_t i = 0; i < _actives.size(); /* no ++ */)
        {
            AbilityBase* ab = _actives[i];
            if (!ab || !ab->IsActive())
            {
                // 防御：不活跃就清理
                unregisterAbility(ab);
                continue; // unregister 会 swap-pop，本 index 需要重新检查
            }

            ctx.self = ab;
            ab->Tick(ctx);
            ctx.self = nullptr;

            if (!ab->IsActive())
            {
                unregisterAbility(ab);
                continue; // 同上
            }

            ++i;
        }

        // 2) 启动候选：你原本的“多次尝试启动”的框架保留，但删掉 started 后 rebuildOwners
        bool tried[128] = {}; // 你原代码里如果不是 128，就保持你当前的大小
        for (;;)
        {
            AbilityBase* best = nullptr;
            int bestIdx = -1;
            int bestScore = 0;

            auto candidateScore = [](AbilityBase* ab) -> int
            {
                // 你原本怎么算 best 的就保留；这里用 0 兜底
                (void)ab;
                return 0;
            };

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
            if (!started)
                continue;

            // [DEL] 这里原来是 rebuildOwners(abilities, count); 现在不需要了
            // 因为 startAbility/cancelAbility 已经增量维护 _resourceOwner / _actives
        }
    }
}
