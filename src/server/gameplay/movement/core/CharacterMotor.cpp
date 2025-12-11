//
// Created by tmsz on 25-12-2.
//

#include "CharacterMotor.h"

namespace movement
{
    namespace
    {
        static Vec3 MoveTowards(const Vec3& current, const Vec3& target, float maxDelta)
        {
            Vec3 delta = target - current;
            float dist = delta.magnitude();
            if (dist <= maxDelta || dist <= 1e-6f)
                return target;
            return current + delta * (maxDelta / dist);
        }
    }

    void CharacterMotor::Step(PlayerState&           state,
                              const MovementCommand& cmd,
                              float                  deltaTime,
                              Vec3&                  outDesiredDisplacement)
    {
        if (deltaTime <= 0.0f)
        {
            outDesiredDisplacement = Vec3::zero();
            return;
        }

        // 1. 视角更新
        state.Yaw   += cmd.LookDeltaYaw;
        state.Pitch += cmd.LookDeltaPitch;

        if (state.Pitch > MaxPitch) state.Pitch = MaxPitch;
        if (state.Pitch < MinPitch) state.Pitch = MinPitch;

        if (state.Yaw > 360.0f) state.Yaw -= 360.0f;
        if (state.Yaw < 0.0f)   state.Yaw += 360.0f;

        // 2. 强制位移：直接输出，清零速度
        if (cmd.HasForcedDisplacement)
        {
            outDesiredDisplacement = cmd.ForcedDisplacement;
            state.Velocity         = Vec3::zero();
            return;
        }

        // 3. 普通运动路径
        Vec3 v = state.Velocity;

        if (state.IsGrounded && v.y <= 0.0f)
        {
            v.y = 0.0f;
        }

        // 3.1 水平速度收敛
        Vec3 currentHorizontal{v.x, 0.0f, v.z};
        Vec3 desiredHorizontal{cmd.DesiredVelocity.x, 0.0f, cmd.DesiredVelocity.z};

        bool  hasInput = (desiredHorizontal.sqrMagnitude() > 0.0001f);
        float accel    = 0.0f;
        float decel    = 0.0f;

        if (state.IsGrounded)
        {
            accel = HorizontalAccelerationGround;
            decel = HorizontalDecelerationGround;
        }
        else
        {
            accel = HorizontalAccelerationAir;
            decel = HorizontalDecelerationAir;
        }

        float usedAccel      = hasInput ? accel : decel;
        float maxSpeedChange = usedAccel * deltaTime;

        Vec3 newHorizontal = MoveTowards(currentHorizontal, desiredHorizontal, maxSpeedChange);
        v.x = newHorizontal.x;
        v.z = newHorizontal.z;

        // 3.2 叠加本帧瞬时冲量
        if (cmd.VelocityImpulse.sqrMagnitude() > 0.0f)
        {
            v += cmd.VelocityImpulse;
        }

        // 3.3 重力
        if (!state.IsGrounded && v.y > -15.0f)
        {
            v.y -= Gravity * deltaTime;
        }

        // 4. 写回速度 & 理想位移
        state.Velocity         = v;
        outDesiredDisplacement = v * deltaTime;
    }
}