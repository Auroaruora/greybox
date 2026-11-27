#include "hillscreen.h"

#include "app.h"
#include "camera/debugcontroller.h"
#include "input/input.h"
#include "utils/motionreader.h"

static const std::string TourFile = "scenes/hill/tour.json";
static const std::string Room = "room";

HillScreen::HillScreen(const std::string &name, const std::string &sceneFile) : Screen(name, sceneFile) {
    m_cameraSystem.add(std::make_unique<DebugController>());

    std::vector<PathKeySource> keys;
    MotionReader::loadPath(TourFile, keys);
    for (const PathKeySource &key : keys) {
        m_tour.add(key.time, CameraPose{key.position, key.look, key.up, key.heightAngle});
    }
}

void HillScreen::enter() {
    m_cameraSystem.playPath(m_tour);
    m_isTouring = true;
}

void HillScreen::handleEvents(App &app) {
    if (!m_isTouring) {
        return;
    }
    if (!m_cameraSystem.isTransitioning() || Input::getButtonDown(Action::Back)) {
        m_isTouring = false;
        app.setScreen(Room);
    }
}
