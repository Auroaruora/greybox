#include "camerapath.h"

#include <algorithm>

static glm::quat toRotation(const glm::vec3 &look, const glm::vec3 &up) {
    glm::vec3 forward = glm::normalize(look);
    glm::vec3 right = glm::cross(forward, up);
    if (glm::dot(right, right) < 1e-8f) {
        glm::vec3 axis(1.0f, 0.0f, 0.0f);
        right = axis - glm::dot(axis, forward) * forward;
    }
    right = glm::normalize(right);
    glm::vec3 cameraUp = glm::cross(right, forward);
    return glm::normalize(glm::quat_cast(glm::mat3(right, cameraUp, -forward)));
}

static PathPoint toPoint(const CameraPose &pose) {
    PathPoint point;
    point.position = pose.position;
    point.rotation = toRotation(pose.look, pose.up);
    point.heightAngle = pose.heightAngle;
    return point;
}

static CameraPose toPose(const PathPoint &point) {
    CameraPose pose;
    pose.position = point.position;
    pose.look = point.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    pose.up = point.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    pose.heightAngle = point.heightAngle;
    return pose;
}

static PathPoint interpolate(const PathPoint &a, const PathPoint &b, float u) {
    PathPoint point;
    point.position = glm::mix(a.position, b.position, u);
    point.rotation = glm::normalize(glm::slerp(a.rotation, b.rotation, u));
    point.heightAngle = glm::mix(a.heightAngle, b.heightAngle, u);
    return point;
}

static bool isBefore(float time, const CameraKey &key) {
    return time < key.time;
}

static PathPoint evaluateBezier(const PathPoint &start, const PathPoint &startHandle,
                                const PathPoint &endHandle, const PathPoint &end, float u) {
    PathPoint a = interpolate(start, startHandle, u);
    PathPoint b = interpolate(startHandle, endHandle, u);
    PathPoint c = interpolate(endHandle, end, u);
    PathPoint ab = interpolate(a, b, u);
    PathPoint bc = interpolate(b, c, u);
    return interpolate(ab, bc, u);
}

void CameraPath::add(float time, const CameraPose &pose) {
    auto position = std::upper_bound(m_keys.begin(), m_keys.end(), time, isBefore);
    m_keys.insert(position, CameraKey{time, pose});
    updateNodes();
}

CameraPose CameraPath::getPose(float time) const {
    if (m_keys.empty()) {
        return CameraPose();
    }
    if (time <= m_keys.front().time) {
        return toPose(m_nodes.front().point);
    }
    if (time >= m_keys.back().time) {
        return toPose(m_nodes.back().point);
    }
    auto next = std::upper_bound(m_keys.begin(), m_keys.end(), time, isBefore);
    auto previous = next - 1;
    float u = (time - previous->time) / (next->time - previous->time);
    size_t i = previous - m_keys.begin();
    const PathNode &from = m_nodes[i];
    const PathNode &to = m_nodes[i + 1];
    return toPose(evaluateBezier(from.point, from.handleOut, to.handleIn, to.point, u));
}

float CameraPath::getDuration() const {
    return m_keys.empty() ? 0.0f : m_keys.back().time;
}

void CameraPath::updateNodes() {
    m_nodes.resize(m_keys.size());
    for (size_t i = 0; i < m_keys.size(); i++) {
        PathPoint point = toPoint(m_keys[i].pose);
        m_nodes[i] = PathNode{point, point, point};
    }

    for (size_t i = 1; i + 1 < m_nodes.size(); i++) {
        float before = m_keys[i].time - m_keys[i - 1].time;
        float after = m_keys[i + 1].time - m_keys[i].time;
        if (before <= 0.0f || after <= 0.0f) {
            continue;
        }
        float ratio = after / before;

        const PathPoint &previous = m_nodes[i - 1].point;
        const PathPoint &current = m_nodes[i].point;
        const PathPoint &next = m_nodes[i + 1].point;

        PathPoint mirrored = interpolate(previous, current, 1.0f + ratio);
        PathPoint aim = interpolate(mirrored, next, 0.5f);
        m_nodes[i].handleOut = interpolate(current, aim, 1.0f / 3.0f);
        m_nodes[i].handleIn = interpolate(current, m_nodes[i].handleOut, -1.0f / ratio);
    }
}
