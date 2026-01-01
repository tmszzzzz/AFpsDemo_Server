//
// Created by tmsz on 25-12-6.
//

// ==== NEW FILE: server/gameplay/movement/sources/NetworkInputMovementSource.cpp ====
#include "NetworkInputMovementSource.h"
#include <cmath>

namespace movement
{

    void NetworkInputMovementSource::UpdateSource(const PlayerState& state,
                                                  MovementCommand&   command,
                                                  float              deltaTime)
    {
        if (!_buffer.frame || deltaTime <= 0.0f)
            return;

        const ServerInputFrame& in = *_buffer.frame;
        const uint32_t buttonsThisTick = in.buttonsThisTick; // down edge
        const uint32_t buttonsDown     = in.buttonsDown;     // hold state
        const uint32_t prevButtonsDown = in.prevButtonsDown; // last tick hold state

        // 1. 平面移动：使用 moveX/moveY + 当前 Yaw 计算期望水平速度
        float h = in.moveX; // -1..1
        float v = in.moveY; // -1..1

        float lenSq = h * h + v * v;
        if (lenSq > 1e-6f)
        {
            float len = std::sqrt(lenSq);
            if (len > 1.0f)
            {
                h /= len;
                v /= len;
            }

            float yawRad = state.Yaw * DEG2RAD;
            Vec3 forward{ std::sin(yawRad), 0.0f, std::cos(yawRad) };
            Vec3 right  { forward.z, 0.0f, -forward.x };

            // 单位化（防止累计误差）
            if (float fLen = forward.magnitude(); fLen > 1e-6f)
                forward = forward * (1.0f / fLen);
            if (float rLen = right.magnitude(); rLen > 1e-6f)
                right = right * (1.0f / rLen);

            Vec3 moveDir = forward * v + right * h;

            // 注意：这里是“在现有 DesiredVelocity 上叠加”，
            // 方便将来有其他 MovementSource 一起贡献速度。
            command.DesiredVelocity += moveDir * _baseMoveSpeed;
        }

        // 2. 视角：这里我们把 InputCommand 的 yaw/pitch 视为“绝对朝向”，
        //    因此转换成相对于当前 state 的增量，让 CharacterMotor 去限制/归一化。
        {
            float yawDelta   = (in.yaw   - state.Yaw)   * _yawSensitivityDeg;
            float pitchDelta = (in.pitch - state.Pitch) * _pitchSensitivityDeg;

            command.LookDeltaYaw   += yawDelta;
            command.LookDeltaPitch += pitchDelta;
        }

        // 3. 按钮型事件：使用 buttonsThisTick 做“一帧 OR”，消费后清零
        if (state.IsGrounded && GetKeyDown(buttonsThisTick, buttonsDown, prevButtonsDown, BUTTON_JUMP))
        {
            command.VelocityImpulse.y += _jumpSpeed;
        }
    }
}

