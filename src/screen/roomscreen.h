#pragma once

#include "screen/screen.h"

class RoomScreen : public Screen {
public:
    RoomScreen(const std::string &name, const std::string &sceneFile);

protected:
    void handleEvents(App &app) override;
};
