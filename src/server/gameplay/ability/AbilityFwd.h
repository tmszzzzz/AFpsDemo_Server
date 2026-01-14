//
// Created by tmsz on 26-1-14.
//

#ifndef DEMO0_SERVER_ABILITYFWD_H
#define DEMO0_SERVER_ABILITYFWD_H
#include <cstdint>

namespace proto { struct GameEvent; }

// Utils 里定义的输入帧；这里只需要指针类型，所以做前向声明即可
struct ServerInputFrame;

namespace ability
{
    struct Context;
    class AbilityBase;
    class Arbiter;
    class AbilitySet;
}

#endif //DEMO0_SERVER_ABILITYFWD_H
