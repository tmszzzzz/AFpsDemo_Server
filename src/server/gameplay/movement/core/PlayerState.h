//
// Created by tmsz on 25-12-2.
//

#ifndef DEMO0_SERVER_PLAYERSTATE_H
#define DEMO0_SERVER_PLAYERSTATE_H

#include "../../../../utils/Utils.h"

namespace movement
{
    struct PlayerState
    {
        Vec3  Position;   // 世界空间位置
        Vec3  Velocity;   // 世界空间速度

        float Yaw  = 0.0f;  // 水平朝向（度）
        float Pitch = 0.0f; // 垂直视角（度）

        bool  IsGrounded = false; // 是否贴地（由碰撞/KCC 决定）
    };
}
#endif //DEMO0_SERVER_PLAYERSTATE_H
