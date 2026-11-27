#pragma once

#include "camera/camerapose.h"

#include <glm/gtc/quaternion.hpp>

#include <vector>

struct CameraKey {
    float time = 0.0f;
    CameraPose pose;
};

struct PathPoint {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float heightAngle = 0.0f;
};

struct PathNode {
    PathPoint point;
    PathPoint handleIn;
    PathPoint handleOut;
};

class CameraPath {
public:
    void add(float time, const CameraPose &pose);

    CameraPose getPose(float time) const;
    float getDuration() const;

private:
    void updateNodes();

    std::vector<CameraKey> m_keys;
    std::vector<PathNode> m_nodes;
};
