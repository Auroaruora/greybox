#include "debugcontroller.h"

#include "input/input.h"

#include <algorithm>
#include <cmath>

DebugController::DebugController() : CameraController("debug") {
}

void DebugController::update(Camera &camera, float deltaTime) {
    walk(camera, deltaTime);
    zoom(camera, Input::getScrollDelta().y * m_scrollZoomSpeed);

    bool dragging = Input::getButton(Action::DebugViewDrag)
        || (Input::getButton(Action::DebugViewEmulate) && Input::getButton(Action::DebugViewEmulateDrag));
    if (!dragging) {
        return;
    }

    glm::vec2 delta = Input::getMouseDelta();
    if (Input::getButton(Action::DebugViewPan)) {
        pan(camera, delta);
    } else if (Input::getButton(Action::DebugViewZoom)) {
        zoom(camera, -delta.y * m_dragZoomSpeed);
    } else {
        orbit(camera, delta);
    }
}

void DebugController::walk(Camera &camera, float deltaTime) const {
    const CameraPose &pose = camera.getPose();
    glm::vec3 forward = pose.look;
    glm::vec3 right = glm::normalize(glm::cross(forward, WorldUp));

    glm::vec3 direction = forward * Input::getAxis(Action::DebugMoveForward, Action::DebugMoveBack)
        + right * Input::getAxis(Action::DebugMoveRight, Action::DebugMoveLeft)
        + WorldUp * Input::getAxis(Action::DebugMoveUp, Action::DebugMoveDown);
    if (glm::length(direction) == 0.0f) {
        return;
    }

    float speed = Input::getButton(Action::DebugMoveFast) ? m_walkSpeed * m_fastFactor : m_walkSpeed;
    camera.setPose(pose.position + glm::normalize(direction) * speed * deltaTime, pose.look, WorldUp);
}

void DebugController::orbit(Camera &camera, const glm::vec2 &delta) const {
    const CameraPose &pose = camera.getPose();
    glm::vec3 pivot = pose.position + pose.look * m_distance;
    float yaw = getYaw(pose.look) + delta.x * m_orbitSpeed;
    float pitch = glm::clamp(getPitch(pose.look) - delta.y * m_orbitSpeed, -m_pitchLimit, m_pitchLimit);
    glm::vec3 direction = getDirection(yaw, pitch);
    camera.setPose(pivot - direction * m_distance, direction, WorldUp);
}

void DebugController::pan(Camera &camera, const glm::vec2 &delta) const {
    const CameraPose &pose = camera.getPose();
    glm::vec3 right = glm::normalize(glm::cross(pose.look, WorldUp));
    glm::vec3 up = glm::cross(right, pose.look);
    glm::vec3 offset = (up * delta.y - right * delta.x) * m_panSpeed * m_distance;
    camera.setPose(pose.position + offset, pose.look, WorldUp);
}

void DebugController::zoom(Camera &camera, float amount) {
    if (amount == 0.0f) {
        return;
    }
    const CameraPose &pose = camera.getPose();
    glm::vec3 pivot = pose.position + pose.look * m_distance;
    m_distance = std::max(m_distance * std::exp(-amount), m_minDistance);
    camera.setPose(pivot - pose.look * m_distance, pose.look, WorldUp);
}
