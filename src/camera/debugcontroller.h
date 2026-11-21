#pragma once

#include "camera/cameracontroller.h"

class DebugController : public CameraController {
public:
    DebugController();

    void update(Camera &camera, float deltaTime) override;

private:
    void walk(Camera &camera, float deltaTime) const;
    void orbit(Camera &camera, const glm::vec2 &delta) const;
    void pan(Camera &camera, const glm::vec2 &delta) const;
    void zoom(Camera &camera, float amount);

    float m_distance = 2.0f;
    float m_minDistance = 0.1f;

    float m_walkSpeed = 2.5f;
    float m_fastFactor = 5.0f;
    float m_orbitSpeed = 0.005f;
    float m_panSpeed = 0.0020f;
    float m_scrollZoomSpeed = 0.1f;
    float m_dragZoomSpeed = 0.005f;
    float m_pitchLimit = glm::radians(89.0f);
};
