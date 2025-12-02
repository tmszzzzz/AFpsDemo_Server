//
// Created by tmsz on 25-12-2.
//

#ifndef DEMO0_SERVER_MOVEMENTSOURCECOLLECTION_H
#define DEMO0_SERVER_MOVEMENTSOURCECOLLECTION_H

#include <memory>
#include <vector>

#include "../sources/IMovementSource.h"

namespace movement
{
    class MovementSourceCollection
    {
    public:
        void AddSource(std::shared_ptr<IMovementSource> src);
        void RemoveSource(const std::shared_ptr<IMovementSource>& src);

        MovementCommand BuildCommand(const PlayerState& state, float deltaTime);

    private:
        std::vector<std::shared_ptr<IMovementSource>> _sources;
    };
}

#endif //DEMO0_SERVER_MOVEMENTSOURCECOLLECTION_H
