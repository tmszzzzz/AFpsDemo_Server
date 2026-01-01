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
#include "../../../GameServer.h"

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
            const ServerInputFrame* frame = nullptr;
        };

        explicit NetworkInputMovementSource(NetInputBuffer buffer)
                : _buffer(buffer)
        {}

        bool IsActive() const override
        {
            return _buffer.frame != nullptr;
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
