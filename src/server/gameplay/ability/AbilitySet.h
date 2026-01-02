//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYSET_H
#define DEMO0_SERVER_ABILITYSET_H


#pragma once

#include <memory>
#include <vector>
#include "AbilityBase.h"
#include "AbilityArbiter.h"

namespace ability
{
    class AbilitySet
    {
    public:
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
