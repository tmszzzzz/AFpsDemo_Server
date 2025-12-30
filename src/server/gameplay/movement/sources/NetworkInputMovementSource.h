//
// Created by tmsz on 25-12-6.
//

#ifndef DEMO0_SERVER_NETWORKINPUTMOVEMENTSOURCE_H
#define DEMO0_SERVER_NETWORKINPUTMOVEMENTSOURCE_H


// ==== NEW FILE: server/gameplay/movement/sources/NetworkInputMovementSource.h ====
#pragma once

#include "IMovementSource.h"
#include "../../../protocol/Messages.h"   // proto::InputCommand
#include "../../../utils/Utils.h"        // Vec3

namespace movement
{
    /// 基于网络 InputCommand 的运动源：
    /// - 读取某个 NetInputBuffer（lastInput + buttonsThisTick）
    /// - 为本帧 MovementCommand 贡献移动 / 视角 / 跳跃等
    class NetworkInputMovementSource : public IMovementSource
    {
    public:
        struct NetInputBuffer
        {
            proto::InputCommand* lastInput      = nullptr; // 最新状态输入
            const uint32_t*      buttonsThisTick = nullptr; // 本 Tick 的按下边沿（down edge）
            const uint32_t*      prevButtonsDown = nullptr; // 上一 Tick 的按住态
        };

        explicit NetworkInputMovementSource(NetInputBuffer buffer)
                : _buffer(buffer)
        {}

        bool IsActive() const override
        {
            // 网络输入源一直是活跃的，只要绑定了缓冲
            return _buffer.lastInput != nullptr && _buffer.buttonsThisTick != nullptr;
        }

        bool AutoRemoveWhenInactive() const override
        {
            return false;
        }

        void UpdateSource(const PlayerState& state,
                          MovementCommand&   command,
                          float              deltaTime) override;

        // 如果将来需要换绑缓冲，可以通过这个接口
        void SetBuffer(NetInputBuffer buffer) { _buffer = buffer; }

    private:
        NetInputBuffer _buffer{};

        // 以后可以从 HeroConfig 注入，这里先写死一组默认参数
        float _baseMoveSpeed      = 6.0f;
        float _jumpSpeed          = 9.0f;
        float _yawSensitivityDeg   = 1.0f; // yaw/pitch 目前使用绝对值 → 用差分，不必太大
        float _pitchSensitivityDeg = 1.0f;
    };
}



#endif //DEMO0_SERVER_NETWORKINPUTMOVEMENTSOURCE_H
