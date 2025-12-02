//
// Created by tmsz on 25-12-2.
//

#include "MovementSourceCollection.h"

#include <algorithm>

namespace movement
{
    void MovementSourceCollection::AddSource(std::shared_ptr<IMovementSource> src)
    {
        if (!src) return;
        _sources.push_back(std::move(src));
    }

    void MovementSourceCollection::RemoveSource(const std::shared_ptr<IMovementSource>& src)
    {
        if (!src) return;

        _sources.erase(
                std::remove_if(_sources.begin(), _sources.end(),
                               [&](const std::shared_ptr<IMovementSource>& p)
                               {
                                   return p == src;
                               }),
                _sources.end());
    }

    MovementCommand MovementSourceCollection::BuildCommand(const PlayerState& state,
                                                           float              deltaTime)
    {
        MovementCommand cmd = MovementCommand::CreateEmpty();

        for (int i = static_cast<int>(_sources.size()) - 1; i >= 0; --i)
        {
            auto& src = _sources[i];
            if (!src)
            {
                _sources.erase(_sources.begin() + i);
                continue;
            }

            if (!src->IsActive() && src->AutoRemoveWhenInactive())
            {
                _sources.erase(_sources.begin() + i);
                continue;
            }

            if (src->IsActive())
            {
                src->UpdateSource(state, cmd, deltaTime);
            }
        }

        return cmd;
    }
}