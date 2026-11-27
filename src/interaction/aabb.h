#pragma once

#include "interaction/ray.h"

#include <glm/glm.hpp>

#include <limits>

class AABB {
public:
    void expand(const glm::vec3 &point);
    void expand(const AABB &other, const glm::mat4 &matrix);
    bool isValid() const;
    glm::vec3 getCenter() const;
    const glm::vec3 &getMin() const;
    const glm::vec3 &getMax() const;
    bool intersect(const Ray &ray, float &distance) const;

private:
    glm::vec3 m_min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 m_max = glm::vec3(std::numeric_limits<float>::lowest());
};
