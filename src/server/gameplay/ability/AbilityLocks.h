//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_ABILITYLOCKS_H
#define DEMO0_SERVER_ABILITYLOCKS_H
#include <cstdint>

namespace ability
{
    enum LockFlags : uint32_t
    {
        Lock_None   = 0,
        Lock_Fire   = 1u << 0,
        Lock_Reload = 1u << 1,
        Lock_Ability= 1u << 2,
    };

    inline bool HasLock(uint32_t locks, uint32_t mask)
    {
        return (locks & mask) != 0;
    }
}

#endif //DEMO0_SERVER_ABILITYLOCKS_H
