//
// Created by tmsz on 25-12-2.
//

#ifndef DEMO0_SERVER_IMOVEMENTSOURCE_H
#define DEMO0_SERVER_IMOVEMENTSOURCE_H
/// <summary>
/// 所有“运动源”的统一接口。
/// 例如：本地输入、Dash、Knockback、Teleport 等。
/// </summary>
#include "MovementCommand.h"
#include "../core/PlayerState.h"

namespace movement
{
    class IMovementSource
    {
    public:
        virtual ~IMovementSource() = default;

        virtual bool IsActive() const = 0;
        virtual bool AutoRemoveWhenInactive() const = 0;

        // 在给定状态下，为本帧的 MovementCommand 贡献自己的部分
        virtual void UpdateSource(const PlayerState& state,
                                  MovementCommand&   command,
                                  float              deltaTime) = 0;
    };
}
#endif //DEMO0_SERVER_IMOVEMENTSOURCE_H
