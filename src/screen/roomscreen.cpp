#include "roomscreen.h"

#include "camera/debugcontroller.h"
#include "camera/focuscontroller.h"
#include "camera/seatedcontroller.h"
#include "debug/debug.h"
#include "desktop/desktop.h"
#include "interaction/lightswitch.h"
#include "interaction/monitor.h"
#include "interaction/mug.h"
#include "story/roomstory.h"

RoomScreen::RoomScreen(const std::string &name, const std::string &sceneFile, const Desktop &desktop)
    : Screen(name, sceneFile) {
    m_cameraSystem.add(std::make_unique<SeatedController>());
    m_cameraSystem.add(std::make_unique<FocusController>());
    if (debug::ToolsEnabled) {
        m_cameraSystem.add(std::make_unique<DebugController>());
    }

    m_interactionSystem.add(std::make_unique<LightSwitch>("deskLamp", "Light_Lamp"));
    m_interactionSystem.add(std::make_unique<Mug>("mug"));
    m_interactionSystem.add(std::make_unique<Monitor>("monitor", desktop.getScreenTexture()));

    m_story = std::make_unique<RoomStory>();
}
