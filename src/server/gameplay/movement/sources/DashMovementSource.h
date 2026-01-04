//
// Created by tmsz on 26-1-3.
//

#ifndef DEMO0_SERVER_DASHMOVEMENTSOURCE_H
#define DEMO0_SERVER_DASHMOVEMENTSOURCE_H

#include "IMovementSource.h"

namespace movement {
    class DashMovementSource : public IMovementSource {
    public:
        void UpdateSource(const PlayerState &state,
                          MovementCommand &command,
                          float deltaTime) override;
        bool IsActive() const;
        bool AutoRemoveWhenInactive() const;

        explicit DashMovementSource(float duration,float speed) : _timer(duration), _speed(speed) {};
        ~DashMovementSource() = default;
    private:
        float _timer;
        float _speed;
        bool _active = true;
    };
}

#endif //DEMO0_SERVER_DASHMOVEMENTSOURCE_H
