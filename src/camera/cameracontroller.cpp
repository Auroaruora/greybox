#include "cameracontroller.h"

#include <cmath>

const glm::vec3 CameraController::WorldUp(0.0f, 1.0f, 0.0f);

CameraController::CameraController(const std::string &name) {
    m_name = name;
}

void CameraController::load(const std::vector<Viewpoint> &viewpoints) {
}

void CameraController::reset(const Camera &camera) {
}

void CameraController::enter(Camera &camera) {
    camera.setPose(getEnterPose(camera));
}

CameraPose CameraController::getEnterPose(const Camera &camera) const {
    return camera.getPose();
}

const std::string &CameraController::getName() const {
    return m_name;
}

float CameraController::getYaw(const glm::vec3 &look) {
    return std::atan2(look.x, -look.z);
}

float CameraController::getPitch(const glm::vec3 &look) {
    return std::asin(glm::clamp(look.y, -1.0f, 1.0f));
}

glm::vec3 CameraController::getDirection(float yaw, float pitch) {
    return glm::vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
}
