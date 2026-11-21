#include "seatedcontroller.h"

#include "input/input.h"

#include <algorithm>

static float approach(float value, float target, float step) {
    if (value < target) {
        return std::min(value + step, target);
    }
    return std::max(value - step, target);
}

SeatedController::SeatedController() : CameraController("seated") {
}

void SeatedController::reset(const Camera &camera) {
    m_seat = camera.getPose();
    m_velocity = glm::vec2(0.0f);
}

void SeatedController::enter(Camera &camera) {
    camera.setPose(getEnterPose(camera));
    m_velocity = glm::vec2(0.0f);
}

CameraPose SeatedController::getEnterPose(const Camera &camera) const {
    CameraPose pose = m_seat;
    pose.up = WorldUp;
    return pose;
}

void SeatedController::update(Camera &camera, float deltaTime) {
    glm::vec2 target(Input::getAxis(Action::LookRight, Action::LookLeft), Input::getAxis(Action::LookUp, Action::LookDown));
    target *= m_turnSpeed;

    float step = m_turnAcceleration * deltaTime;
    m_velocity.x = approach(m_velocity.x, target.x, step);
    m_velocity.y = approach(m_velocity.y, target.y, step);

    const CameraPose &pose = camera.getPose();
    float yaw = getYaw(pose.look) + m_velocity.x * deltaTime;
    float pitch = getPitch(pose.look) + m_velocity.y * deltaTime;

    float limited = glm::clamp(pitch, -m_downLimit, m_upLimit);
    if (limited != pitch) {
        m_velocity.y = 0.0f;
    }

    camera.setPose(pose.position, getDirection(yaw, limited), WorldUp);
}
