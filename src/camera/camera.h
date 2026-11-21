#pragma once

#include "camera/camerapose.h"
#include "interaction/ray.h"

#include <glm/glm.hpp>

class Camera {
public:
    Camera();

    void setPose(const CameraPose &pose);
    void setPose(const glm::vec3 &position, const glm::vec3 &look, const glm::vec3 &up);
    void setAspectRatio(float aspect);

    const CameraPose &getPose() const;
    const glm::mat4 &getViewMatrix() const;
    const glm::mat4 &getProjectionMatrix() const;
    Ray getRayThrough(const glm::vec2 &ndc) const;

private:
    void updateView();
    void updateProjection();

    CameraPose m_pose;
    float m_aspect = 1.0f;
    float m_near = 0.1f;
    float m_far = 100.0f;

    glm::mat4 m_view = glm::mat4(1.0f);
    glm::mat4 m_inverseView = glm::mat4(1.0f);
    glm::mat4 m_projection = glm::mat4(1.0f);
};
