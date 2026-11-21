#include "camera.h"

#include <cmath>

Camera::Camera() {
    updateView();
    updateProjection();
}

void Camera::setPose(const CameraPose &pose) {
    m_pose.position = pose.position;
    m_pose.look = glm::normalize(pose.look);
    m_pose.up = glm::normalize(pose.up);
    m_pose.heightAngle = pose.heightAngle;
    updateView();
    updateProjection();
}

void Camera::setPose(const glm::vec3 &position, const glm::vec3 &look, const glm::vec3 &up) {
    m_pose.position = position;
    m_pose.look = glm::normalize(look);
    m_pose.up = glm::normalize(up);
    updateView();
}

void Camera::setAspectRatio(float aspect) {
    if (aspect <= 0.0f) {
        return;
    }
    m_aspect = aspect;
    updateProjection();
}

const CameraPose &Camera::getPose() const {
    return m_pose;
}

const glm::mat4 &Camera::getViewMatrix() const {
    return m_view;
}

const glm::mat4 &Camera::getProjectionMatrix() const {
    return m_projection;
}

void Camera::updateView() {
    glm::vec3 w = glm::normalize(-m_pose.look);
    glm::vec3 v = glm::normalize(m_pose.up - glm::dot(m_pose.up, w) * w);
    glm::vec3 u = glm::cross(v, w);

    glm::mat4 rotation(1.0f);
    rotation[0][0] = u.x; rotation[1][0] = u.y; rotation[2][0] = u.z;
    rotation[0][1] = v.x; rotation[1][1] = v.y; rotation[2][1] = v.z;
    rotation[0][2] = w.x; rotation[1][2] = w.y; rotation[2][2] = w.z;

    glm::mat4 translation(1.0f);
    translation[3][0] = -m_pose.position.x;
    translation[3][1] = -m_pose.position.y;
    translation[3][2] = -m_pose.position.z;

    m_view = rotation * translation;
    m_inverseView = glm::inverse(m_view);
}

void Camera::updateProjection() {
    float tanHalfHeight = std::tan(m_pose.heightAngle / 2.0f);
    float tanHalfWidth = m_aspect * tanHalfHeight;

    glm::mat4 scale(1.0f);
    scale[0][0] = 1.0f / (m_far * tanHalfWidth);
    scale[1][1] = 1.0f / (m_far * tanHalfHeight);
    scale[2][2] = 1.0f / m_far;

    float c = -m_near / m_far;
    glm::mat4 unhinge(1.0f);
    unhinge[2][2] = 1.0f / (1.0f + c);
    unhinge[3][2] = -c / (1.0f + c);
    unhinge[2][3] = -1.0f;
    unhinge[3][3] = 0.0f;

    glm::mat4 remap(1.0f);
    remap[2][2] = -2.0f;
    remap[3][2] = -1.0f;

    m_projection = remap * unhinge * scale;
}

Ray Camera::getRayThrough(const glm::vec2 &ndc) const {
    float tanHalfHeight = std::tan(m_pose.heightAngle / 2.0f);
    glm::vec3 direction(ndc.x * m_aspect * tanHalfHeight, ndc.y * tanHalfHeight, -1.0f);

    Ray ray;
    ray.origin = m_pose.position;
    ray.direction = glm::normalize(glm::vec3(m_inverseView * glm::vec4(direction, 0.0f)));
    return ray;
}