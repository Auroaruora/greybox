#pragma once

#include "camera/camerapath.h"
#include "screen/screen.h"

class HillScreen : public Screen {
public:
    HillScreen(const std::string &name, const std::string &sceneFile);

    void enter() override;

protected:
    void handleEvents(App &app) override;

private:
    CameraPath m_tour;
    bool m_isTouring = false;
};
