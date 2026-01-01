//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYSLOT_H
#define DEMO0_SERVER_ABILITYSLOT_H

#include <cstdint>

namespace ability
{
    enum class Slot : uint8_t
    {
        None = 0,
        Primary   = 1, // LMB
        Secondary = 2, // RMB
        Skill1    = 3, // E
        Skill2    = 4, // Shift
        Ultimate  = 5, // Q
        Reload    = 6, // R
    };
}

#endif //DEMO0_SERVER_ABILITYSLOT_H
