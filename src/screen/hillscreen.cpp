#include "hillscreen.h"

#include "camera/debugcontroller.h"

HillScreen::HillScreen(const std::string &name, const std::string &sceneFile) : Screen(name, sceneFile) {
    m_cameraSystem.add(std::make_unique<DebugController>());
}
