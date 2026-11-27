#include "roomscreen.h"

#include "app.h"
#include "camera/debugcontroller.h"
#include "camera/focuscontroller.h"
#include "camera/seatedcontroller.h"
#include "debug/debug.h"
#include "input/input.h"
#include "interaction/lightswitch.h"
#include "interaction/monitor.h"
#include "interaction/mug.h"

static const std::string MonitorName = "monitor";
static const std::string ScreenImage = "ui/monitor/mountains.png";
static const std::string Focus = "focus";
static const std::string Seated = "seated";
static const std::string Hill = "hill";
static const float FocusTransitionTime = 0.5f;

RoomScreen::RoomScreen(const std::string &name, const std::string &sceneFile) : Screen(name, sceneFile) {
    m_cameraSystem.add(std::make_unique<SeatedController>());
    m_cameraSystem.add(std::make_unique<FocusController>());
    if (debug::ToolsEnabled) {
        m_cameraSystem.add(std::make_unique<DebugController>());
    }

    m_interactionSystem.add(std::make_unique<LightSwitch>("deskLamp", "Light_Lamp"));
    m_interactionSystem.add(std::make_unique<Mug>("mug"));
    m_interactionSystem.add(std::make_unique<Monitor>(MonitorName, ScreenImage));
}

void RoomScreen::handleEvents(App &app) {
    for (const GameEvent &event : m_interactionSystem.getEvents()) {
        if (event.type != EventType::Click || event.source != MonitorName) {
            continue;
        }
        if (m_cameraSystem.getControllerName() == Focus) {
            app.setScreen(Hill);
        } else {
            m_cameraSystem.setController(Focus, FocusTransitionTime);
        }
    }

    if (Input::getButtonDown(Action::Back) && m_cameraSystem.getControllerName() == Focus) {
        m_cameraSystem.setController(Seated, FocusTransitionTime);
    }
}
