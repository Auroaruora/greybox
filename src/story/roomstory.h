#pragma once

#include "story/story.h"

class RoomStory : public Story {
public:
    void update(CameraSystem &cameraSystem, const std::vector<GameEvent> &events) override;
};
