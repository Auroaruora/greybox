#include "focuscontroller.h"

#include "debug/debug.h"

static const std::string FocusViewpoint = "Cam_Focus";

FocusController::FocusController() : CameraController("focus") {
}

void FocusController::load(const std::vector<Viewpoint> &viewpoints) {
    m_hasFocus = false;
    for (const Viewpoint &viewpoint : viewpoints) {
        if (viewpoint.name == FocusViewpoint) {
            m_focus = viewpoint.pose;
            m_hasFocus = true;
            return;
        }
    }
    LOG_INFO(getName() << ": no viewpoint named " << FocusViewpoint);
}

void FocusController::enter(Camera &camera) {
    camera.setPose(getEnterPose(camera));
}

CameraPose FocusController::getEnterPose(const Camera &camera) const {
    return m_hasFocus ? m_focus : camera.getPose();
}

void FocusController::update(Camera &camera, float deltaTime) {
}
