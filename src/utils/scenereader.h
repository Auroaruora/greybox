#pragma once

#include "scenedata.h"

#include <glm/glm.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

struct SceneSettings {
    ShadingModel shading = ShadingModel::Phong;
    std::optional<glm::dvec3> specular;
    std::optional<double> shininess;
    std::optional<double> far;
    std::string startLook;
    std::string startViewpoint;
    std::string startState;
};

struct LayoutObject {
    std::string name;
    std::string asset;
    glm::dvec3 position = glm::dvec3(0.0);
    glm::dvec3 rotation = glm::dvec3(0.0);
    std::string group;
};

struct LookChoice {
    std::optional<std::string> shape;
    std::optional<std::string> colour;
    std::optional<glm::dvec3> scale;
};

struct GlowSetting {
    double strength = 0.0;
    glm::dvec3 color = glm::dvec3(1.0);
    std::optional<glm::dvec3> base;
};

struct LightSource {
    std::string name;
    std::string type;
    std::string attach;
    std::string glowPart;
    std::optional<glm::dvec3> position;
    std::optional<glm::dvec3> direction;
    double angle = 0.0;
    double penumbra = 0.0;
};

struct LightSetting {
    double intensity = 0.0;
    glm::dvec3 color = glm::dvec3(1.0);
    std::optional<glm::dvec3> direction;
    std::optional<GlowSetting> glow;
};

struct StateSource {
    std::string name;
    glm::vec3 ambient = glm::vec3(0.0f);
    std::optional<SkyColors> sky;
    std::optional<glm::vec3> toonShade;
    std::map<std::string, LightSetting> lights;
    std::map<std::string, GlowSetting> glow;
};

struct ViewSource {
    std::string name;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 look = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float heightAngle = 0.0f;
};

struct SceneSource {
    SceneSettings settings;
    std::vector<LayoutObject> layout;
    std::map<std::string, LookChoice> look;
    std::vector<LightSource> lights;
    std::vector<StateSource> states;
    std::vector<ViewSource> viewpoints;
};

namespace SceneReader {
bool load(const std::string &lookFile, SceneSource &source);
}
