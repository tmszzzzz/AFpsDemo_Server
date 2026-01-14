//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYSET_H
#define DEMO0_SERVER_ABILITYSET_H

#include <memory>
#include <vector>
#include "AbilityFwd.h"

namespace ability
{
    class AbilitySet
    {
    public:
        AbilitySet();
        ~AbilitySet();                 // 关键：只声明，不在头里 default

        AbilitySet(const AbilitySet&) = delete;
        AbilitySet& operator=(const AbilitySet&) = delete;
        AbilitySet(AbilitySet&&) noexcept = default;
        AbilitySet& operator=(AbilitySet&&) noexcept = default;

        void Add(std::unique_ptr<AbilityBase> ab);
        void Tick(Context& ctx);

    private:
        std::vector<std::unique_ptr<AbilityBase>> _abs;
        std::vector<AbilityBase*> _raw;
        std::unique_ptr<Arbiter> _arbiter;

        void rebuildRaw();
    };
}


#endif //DEMO0_SERVER_ABILITYSET_H
