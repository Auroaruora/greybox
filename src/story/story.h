#pragma once

#include "event/gameevent.h"

#include <vector>

class CameraSystem;

class Story {
public:
    virtual ~Story() = default;

    virtual void update(CameraSystem &cameraSystem, const std::vector<GameEvent> &events) = 0;
};
