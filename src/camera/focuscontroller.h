#pragma once

#include "camera/cameracontroller.h"

class FocusController : public CameraController {
public:
    FocusController();

    void load(const std::vector<Viewpoint> &viewpoints) override;
    void enter(Camera &camera) override;
    CameraPose getEnterPose(const Camera &camera) const override;
    void update(Camera &camera, float deltaTime) override;

private:
    CameraPose m_focus;
    bool m_hasFocus = false;
};
