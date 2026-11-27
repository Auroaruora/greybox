#pragma once

#include "camera/camera.h"
#include "utils/sceneparser.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

class CameraController {
public:
    explicit CameraController(const std::string &name);
    virtual ~CameraController() = default;

    virtual void load(const std::vector<Viewpoint> &viewpoints);
    virtual void reset(const Camera &camera);
    virtual void enter(Camera &camera);
    virtual CameraPose getEnterPose(const Camera &camera) const;
    virtual void update(Camera &camera, float deltaTime) = 0;
    const std::string &getName() const;

protected:
    static const glm::vec3 WorldUp;

    static float getYaw(const glm::vec3 &look);
    static float getPitch(const glm::vec3 &look);
    static glm::vec3 getDirection(float yaw, float pitch);

private:
    std::string m_name;
};
