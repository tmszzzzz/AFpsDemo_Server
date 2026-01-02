//
// Created by tmsz on 25-12-2.
//

#include "HeroCore.h"

#include <algorithm>    // std::clamp

namespace hero
{
    HeroCore::HeroCore(HeroId heroId,
                       const Vec3& spawnPos,
                       const HeroConfig& config)
    {
        // 配置 CharacterMotor
        _motor.Gravity  = config.Gravity;
        _motor.MinPitch = config.MinPitch;
        _motor.MaxPitch = config.MaxPitch;

        _motor.HorizontalAccelerationGround  = config.HorizontalAccelerationGround;
        _motor.HorizontalDecelerationGround  = config.HorizontalDecelerationGround;
        _motor.HorizontalAccelerationAir     = config.HorizontalAccelerationAir;
        _motor.HorizontalDecelerationAir     = config.HorizontalDecelerationAir;

        // MovementSourceCollection 默认构造即可
         _movementSources = movement::MovementSourceCollection();

        // 初始化状态
        _state.HeroId = heroId;
        _state.playerState.Position   = spawnPos;
        _state.playerState.Velocity   = Vec3::zero();
        _state.playerState.Yaw        = 0.0f;
        _state.playerState.Pitch      = 0.0f;
        _state.playerState.IsGrounded = false;

        _state.MaxHp = config.MaxHp;
        _state.Hp    = config.MaxHp;
    }

    void HeroCore::TickMovement(float                      deltaTime,
                                movement::MovementCommand& outCommand,
                                Vec3&                      outDesiredDisplacement)
    {
        // 由所有 MovementSource 合成 MovementCommand
        outCommand = _movementSources.BuildCommand(_state.playerState, deltaTime);

        // 交给 CharacterMotor 计算“理想位移”
        _motor.Step(_state.playerState, outCommand, deltaTime, outDesiredDisplacement);

        // 注意：不在这里改 Position/IsGrounded，碰撞由外层 KCC 处理后回写
    }

    void HeroCore::ApplyDamage(float amount)
    {
        float newHp = _state.Hp - amount;
        newHp = std::clamp(newHp, 0.0f, _state.MaxHp);
        _state.Hp = newHp;
    }

    void HeroCore::AddMovementSource(std::shared_ptr<movement::IMovementSource> src)
    {
        _movementSources.AddSource(std::move(src));
    }
}

