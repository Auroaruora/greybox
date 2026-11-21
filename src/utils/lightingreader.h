#pragma once

#include "scenedata.h"

#include <glm/glm.hpp>

#include <map>
#include <string>
#include <vector>

struct LightingGroup {
    std::string name;
    glm::vec3 ambient;
    std::vector<SceneLightData> lights;
    std::map<std::string, SceneColor> emissive;
};

class LightingReader {
public:
    LightingReader(const std::string &filename);

    bool readJSON();

    const std::vector<LightingGroup> &getGroups() const;

private:
    std::string m_filename;
    std::vector<LightingGroup> m_groups;
};
