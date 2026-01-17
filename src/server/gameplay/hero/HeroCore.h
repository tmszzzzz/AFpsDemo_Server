//
// Created by tmsz on 25-12-2.
//

#ifndef DEMO0_SERVER_HEROCORE_H
#define DEMO0_SERVER_HEROCORE_H

#include <memory>
#include "HeroDefines.h"
#include "../../../utils/Utils.h"
#include "../movement/sources/IMovementSource.h"
#include "../movement/core/MovementCommand.h"
#include "../movement/core/CharacterMotor.h"
#include "../movement/core/MovementSourceCollection.h"

namespace hero
{
    class HeroCore
    {
    public:
        HeroCore(HeroId heroId,
                 const Vec3& spawnPos,
                 const HeroConfig& config);

        // 访问状态
        HeroState&       GetState()       { return _state; }
        const HeroState& GetState() const { return _state; }

        movement::PlayerState&       Movement()       { return _state.playerState; }
        const movement::PlayerState& Movement() const { return _state.playerState; }
        float ViewHeight() const { return _viewHeight; }

        // 一帧运动逻辑（不做碰撞，只算理想位移）
        void TickMovement(float                        deltaTime,
                          movement::MovementCommand&   outCommand,
                          Vec3&                        outDesiredDisplacement);

        // 施加伤害
        void ApplyDamage(float amount);

        // 注册运动源（本地输入、dash、击退等）
        void AddMovementSource(std::shared_ptr<movement::IMovementSource> src);

    private:
        HeroState                        _state;
        movement::CharacterMotor         _motor;
        movement::MovementSourceCollection _movementSources;
        float _viewHeight = 1.6f;
    };
}


#endif //DEMO0_SERVER_HEROCORE_H
