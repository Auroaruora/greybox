#pragma once

#include <glm/glm.hpp>

struct CameraPose {
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 look = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float heightAngle = glm::radians(30.0f);
};
