#pragma once

#include "camera/cameracontroller.h"

class SeatedController : public CameraController {
public:
    SeatedController();

    void reset(const Camera &camera) override;
    void enter(Camera &camera) override;
    CameraPose getEnterPose(const Camera &camera) const override;
    void update(Camera &camera, float deltaTime) override;

private:
    CameraPose m_seat;
    glm::vec2 m_velocity = glm::vec2(0.0f);

    float m_turnSpeed = glm::radians(100.0f);
    float m_turnAcceleration = glm::radians(400.0f);
    float m_upLimit = glm::radians(50.0f);
    float m_downLimit = glm::radians(50.0f);
};
