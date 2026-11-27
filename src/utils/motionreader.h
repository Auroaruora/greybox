#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct MotionSource {
    std::string object;
    std::string part;
    std::string pivot;
    glm::vec3 spin = glm::vec3(0.0f);
    glm::vec3 sway = glm::vec3(0.0f);
    float period = 1.0f;
};

struct PathKeySource {
    float time = 0.0f;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 look = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float heightAngle = 0.0f;
};

namespace MotionReader {
bool loadMotions(const std::string &file, std::vector<MotionSource> &motions);
bool loadPath(const std::string &file, std::vector<PathKeySource> &keys);
}
