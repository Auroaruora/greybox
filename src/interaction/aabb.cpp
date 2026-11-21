#include "aabb.h"

#include <algorithm>

static const float Padding = 0.005f;

void AABB::expand(const glm::vec3 &point) {
    m_min = glm::min(m_min, point);
    m_max = glm::max(m_max, point);
}

void AABB::expand(const AABB &other, const glm::mat4 &matrix) {
    glm::vec3 low = other.m_min - Padding;
    glm::vec3 high = other.m_max + Padding;
    for (int corner = 0; corner < 8; corner++) {
        glm::vec3 point((corner & 1) ? high.x : low.x, (corner & 2) ? high.y : low.y, (corner & 4) ? high.z : low.z);
        expand(glm::vec3(matrix * glm::vec4(point, 1.0f)));
    }
}

bool AABB::isValid() const {
    return m_min.x <= m_max.x;
}

glm::vec3 AABB::getCenter() const {
    return (m_min + m_max) * 0.5f;
}

bool AABB::intersect(const Ray &ray, float &distance) const {
    float enter = 0.0f;
    float leave = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; axis++) {
        float low = m_min[axis] - Padding;
        float high = m_max[axis] + Padding;
        float origin = ray.origin[axis];
        float direction = ray.direction[axis];

        if (direction == 0.0f) {
            if (origin < low || origin > high) {
                return false;
            }
            continue;
        }

        float first = (low - origin) / direction;
        float second = (high - origin) / direction;
        if (first > second) {
            std::swap(first, second);
        }
        enter = std::max(enter, first);
        leave = std::min(leave, second);
        if (enter > leave) {
            return false;
        }
    }

    distance = enter;
    return true;
}
