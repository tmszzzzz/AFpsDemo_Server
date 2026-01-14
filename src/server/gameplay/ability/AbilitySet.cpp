//
// Created by tmsz on 26-1-1.
//

#include "AbilitySet.h"
#include "AbilityArbiter.h"
#include "AbilityBase.h"

namespace ability
{
    AbilitySet::AbilitySet() = default;
    AbilitySet::~AbilitySet() = default;

    void AbilitySet::rebuildRaw()
    {
        _raw.clear();
        _raw.reserve(_abs.size());
        for (auto& p : _abs) _raw.push_back(p.get());
    }

    void AbilitySet::Add(std::unique_ptr<AbilityBase> ab)
    {
        if (!_arbiter) _arbiter = std::make_unique<Arbiter>();
        _abs.emplace_back(std::move(ab));
        rebuildRaw();
    }

    void AbilitySet::Tick(Context& ctx)
    {
        if (!_arbiter) return;
        _arbiter->Tick(ctx, _raw.data(), (int)_raw.size());
    }
}