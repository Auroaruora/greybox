#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

class MeshCache;
class Scene;

class Motions {
public:
    void load(const Scene &scene, MeshCache &meshCache, const std::string &file);
    void update(Scene &scene, float deltaTime);

private:
    struct Motion {
        std::vector<int> shapes;
        std::vector<glm::mat4> bases;
        glm::vec3 pivot = glm::vec3(0.0f);
        glm::mat3 frame = glm::mat3(1.0f);
        glm::vec3 spin = glm::vec3(0.0f);
        glm::vec3 sway = glm::vec3(0.0f);
        float period = 1.0f;
    };

    std::vector<Motion> m_motions;
    float m_time = 0.0f;
};
