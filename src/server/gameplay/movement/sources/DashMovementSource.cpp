//
// Created by tmsz on 26-1-3.
//

#include "DashMovementSource.h"

namespace movement {
    void DashMovementSource::UpdateSource(const movement::PlayerState &state, movement::MovementCommand &command,
                                          float deltaTime) {
        float yawRad = state.Yaw * DEG2RAD;
        Vec3 forward{ std::sin(yawRad), 0.0f, std::cos(yawRad) };
        if(_timer > 0) {
            float t = std::min(deltaTime,_timer);
            _timer -= deltaTime;
            command.HasForcedDisplacement = true;
            command.ForcedDisplacement = forward * _speed * t;
        }else {
            _active = false;
        }
    }
    bool DashMovementSource::IsActive() const { return _active; }
    bool DashMovementSource::AutoRemoveWhenInactive() const { return true; }
}
