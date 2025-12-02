//
// Created by tmsz on 25-12-2.
//

#ifndef DEMO0_SERVER_CHARACTERMOTOR_H
#define DEMO0_SERVER_CHARACTERMOTOR_H

#include "PlayerState.h"
#include "MovementCommand.h"

namespace movement
{
    class CharacterMotor
    {
    public:
        float Gravity  = 20.0f;   // 世界重力加速度（m/s^2，向下）
        float MinPitch = -89.0f;  // 视角约束（度）
        float MaxPitch =  89.0f;

        // 水平速度收敛参数
        float HorizontalAccelerationGround  = 80.0f;
        float HorizontalDecelerationGround  = 60.0f;
        float HorizontalAccelerationAir     = 30.0f;
        float HorizontalDecelerationAir     = 20.0f;

        // 根据 MovementCommand 推进状态，输出“理想位移”（未做碰撞裁剪）
        void Step(PlayerState&            state,
                  const MovementCommand&  cmd,
                  float                   deltaTime,
                  Vec3&                   outDesiredDisplacement);
    };
}

#endif //DEMO0_SERVER_CHARACTERMOTOR_H
