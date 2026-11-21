#pragma once

#include "screen/screen.h"

class Desktop;

class RoomScreen : public Screen {
public:
    RoomScreen(const std::string &name, const std::string &sceneFile, const Desktop &desktop);
};
