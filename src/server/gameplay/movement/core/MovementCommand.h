//
// Created by tmsz on 25-12-2.
//

#ifndef DEMO0_SERVER_MOVEMENTCOMMAND_H
#define DEMO0_SERVER_MOVEMENTCOMMAND_H

#include "../../../../utils/Utils.h"

namespace movement
{
    struct MovementCommand
    {
        Vec3  DesiredVelocity = Vec3::zero();      // 本帧期望的世界空间连续速度
        Vec3  VelocityImpulse = Vec3::zero();      // 本帧额外叠加到速度上的瞬时冲量
        Vec3  ForcedDisplacement = Vec3::zero();   // 本帧的强制位移
        bool  HasForcedDisplacement = false;

        float LookDeltaYaw   = 0.0f;               // 视角变化（度）
        float LookDeltaPitch = 0.0f;

        static MovementCommand CreateEmpty()
        {
            MovementCommand cmd; // 默认构造已置零
            return cmd;
        }
    };
}

#endif //DEMO0_SERVER_MOVEMENTCOMMAND_H
