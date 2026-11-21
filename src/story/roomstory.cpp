#include "roomstory.h"

#include "camera/camerasystem.h"
#include "input/input.h"

static const std::string Monitor = "monitor";
static const std::string Focus = "focus";
static const std::string Seated = "seated";
static const float FocusTransitionTime = 0.5f;

void RoomStory::update(CameraSystem &cameraSystem, const std::vector<GameEvent> &events) {
    for (const GameEvent &event : events) {
        if (event.type == EventType::Click && event.source == Monitor) {
            cameraSystem.setController(Focus, FocusTransitionTime);
        }
    }

    if (Input::getButtonDown(Action::Back) && cameraSystem.getControllerName() == Focus) {
        cameraSystem.setController(Seated, FocusTransitionTime);
    }
}
