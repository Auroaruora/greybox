#include "lightingreader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

static glm::vec3 readVec3(const json &value) {
    return glm::vec3(value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>());
}

static SceneLightData readLight(const json &data, int id) {
    SceneLightData light;
    light.id = id;
    light.name = data.value("name", std::string());
    light.emitter = data.value("emitter", std::string());
    light.color = SceneColor(readVec3(data.at("color")), 1.0f);
    light.function = data.contains("attenuationCoeff") ? readVec3(data.at("attenuationCoeff")) : glm::vec3(1.0f, 0.0f, 0.0f);
    light.pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    light.dir = glm::vec4(0.0f);
    light.penumbra = 0.0f;
    light.angle = 0.0f;
    light.width = 0.0f;
    light.height = 0.0f;

    std::string type = data.at("type").get<std::string>();
    if (type == "directional") {
        light.type = LightType::LIGHT_DIRECTIONAL;
    } else if (type == "point") {
        light.type = LightType::LIGHT_POINT;
    } else if (type == "spot") {
        light.type = LightType::LIGHT_SPOT;
        light.penumbra = glm::radians(data.at("penumbra").get<float>());
        light.angle = glm::radians(data.at("angle").get<float>());
    } else {
        throw std::runtime_error("unknown light type: " + type);
    }

    if (light.type != LightType::LIGHT_POINT) {
        light.dir = glm::vec4(readVec3(data.at("direction")), 0.0f);
    }
    if (light.type != LightType::LIGHT_DIRECTIONAL) {
        light.pos = glm::vec4(readVec3(data.at("position")), 1.0f);
    }
    return light;
}

LightingReader::LightingReader(const std::string &filename) {
    m_filename = filename;
}

bool LightingReader::readJSON() {
    std::ifstream file(Paths::getAssetPath(m_filename));
    if (!file.is_open()) {
        LOG_ERROR("cannot open " << m_filename);
        return false;
    }

    try {
        json groups = json::parse(file);
        for (const json &data : groups) {
            LightingGroup group;
            group.name = data.at("name").get<std::string>();
            group.ambient = readVec3(data.at("ambient"));

            int lightId = 0;
            for (const json &light : data.at("lights")) {
                group.lights.push_back(readLight(light, lightId++));
            }
            if (data.contains("emissive")) {
                for (const auto &[model, colour] : data.at("emissive").items()) {
                    group.emissive[model] = SceneColor(readVec3(colour), 1.0f);
                }
            }
            m_groups.push_back(group);
        }
    } catch (const std::exception &error) {
        LOG_ERROR(m_filename << ": " << error.what());
        m_groups.clear();
        return false;
    }
    return true;
}

const std::vector<LightingGroup> &LightingReader::getGroups() const {
    return m_groups;
}
