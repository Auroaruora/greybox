#include "scenereader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

using json = nlohmann::ordered_json;

static json readJson(const std::string &path) {
    std::ifstream file(Paths::getAssetPath(path));
    if (!file.is_open()) {
        throw std::runtime_error("cannot open " + path);
    }
    return json::parse(file);
}

static glm::dvec3 readVec3(const json &value) {
    return glm::dvec3(value.at(0).get<double>(), value.at(1).get<double>(), value.at(2).get<double>());
}

static std::optional<glm::dvec3> readOptionalVec3(const json &object, const char *key) {
    return object.contains(key) ? std::optional<glm::dvec3>(readVec3(object.at(key))) : std::nullopt;
}

static ShadingModel readShading(const std::string &name) {
    if (name == "phong") {
        return ShadingModel::Phong;
    }
    throw std::runtime_error("unknown shading: " + name);
}

static SceneSettings readSettings(const json &data) {
    SceneSettings settings;
    settings.shading = readShading(data.value("shading", "phong"));
    if (data.contains("material")) {
        const json &material = data.at("material");
        settings.specular = readOptionalVec3(material, "specular");
        if (material.contains("shininess")) {
            settings.shininess = material.at("shininess").get<double>();
        }
    }
    if (data.contains("camera") && data.at("camera").contains("far")) {
        settings.far = data.at("camera").at("far").get<double>();
    }
    const json &start = data.at("start");
    settings.startLook = start.at("look").get<std::string>();
    settings.startViewpoint = start.at("viewpoint").get<std::string>();
    settings.startState = start.at("state").get<std::string>();
    return settings;
}

static std::vector<LayoutObject> readLayout(const json &data) {
    std::vector<LayoutObject> objects;
    for (const auto &[name, entry] : data.at("objects").items()) {
        LayoutObject object;
        object.name = name;
        object.asset = entry.at("asset").get<std::string>();
        object.position = readVec3(entry.at("position"));
        object.rotation = readVec3(entry.at("rotation"));
        object.group = entry.value("group", "");
        objects.push_back(object);
    }
    return objects;
}

static std::map<std::string, LookChoice> readLook(const json &data) {
    std::map<std::string, LookChoice> look;
    for (const auto &[name, entry] : data.at("objects").items()) {
        LookChoice choice;
        if (entry.contains("shape")) {
            choice.shape = entry.at("shape").get<std::string>();
        }
        if (entry.contains("color")) {
            choice.colour = entry.at("color").get<std::string>();
        }
        choice.scale = readOptionalVec3(entry, "scale");
        look[name] = choice;
    }
    return look;
}

static GlowSetting readGlow(const json &data) {
    GlowSetting glow;
    glow.strength = data.value("strength", 0.0);
    if (data.contains("color")) {
        glow.color = readVec3(data.at("color"));
    }
    glow.base = readOptionalVec3(data, "base");
    return glow;
}

static LightSource readLight(const std::string &name, const json &data) {
    LightSource light;
    light.name = name;
    light.type = data.at("type").get<std::string>();
    light.attach = data.value("attach", "");
    light.glowPart = data.value("glowPart", "");
    light.position = readOptionalVec3(data, "position");
    light.direction = readOptionalVec3(data, "direction");
    light.angle = data.value("angle", 0.0);
    light.penumbra = data.value("penumbra", 0.0);
    return light;
}

static LightSetting readLightSetting(const json &data) {
    LightSetting setting;
    setting.intensity = data.value("intensity", 0.0);
    if (data.contains("color")) {
        setting.color = readVec3(data.at("color"));
    }
    setting.direction = readOptionalVec3(data, "direction");
    if (data.contains("glow")) {
        setting.glow = readGlow(data.at("glow"));
    }
    return setting;
}

static LightingState readState(const std::string &name, const json &data) {
    LightingState state;
    state.name = name;
    if (data.contains("environment") && data.at("environment").contains("world")) {
        state.world = readVec3(data.at("environment").at("world"));
    }
    if (data.contains("lights")) {
        for (const auto &[light, setting] : data.at("lights").items()) {
            state.lights[light] = readLightSetting(setting);
        }
    }
    if (data.contains("glow")) {
        for (const auto &[object, parts] : data.at("glow").items()) {
            for (const auto &[part, glow] : parts.items()) {
                state.glow[object + "_" + part] = readGlow(glow);
            }
        }
    }
    return state;
}

static std::vector<ViewSource> readViewpoints(const json &data) {
    std::vector<ViewSource> views;
    for (const auto &[name, entry] : data.items()) {
        ViewSource view;
        view.name = name;
        view.position = readVec3(entry.at("position"));
        view.look = readVec3(entry.at("look"));
        view.up = readVec3(entry.at("up"));
        view.heightAngle = entry.at("heightAngle").get<double>();
        views.push_back(view);
    }
    return views;
}

bool SceneReader::load(const std::string &lookFile, SceneSource &source) {
    try {
        std::string folder = std::filesystem::path(lookFile).parent_path().parent_path().string();
        source.settings = readSettings(readJson(folder + "/scene.json"));
        source.layout = readLayout(readJson(folder + "/layout.json"));
        source.look = readLook(readJson(lookFile));
        json lighting = readJson(folder + "/lighting.json");
        for (const auto &[name, light] : lighting.at("lights").items()) {
            source.lights.push_back(readLight(name, light));
        }
        for (const auto &[name, state] : lighting.at("states").items()) {
            source.states.push_back(readState(name, state));
        }
        source.viewpoints = readViewpoints(readJson(folder + "/viewpoints.json"));
    } catch (const std::exception &error) {
        LOG_ERROR(lookFile << ": " << error.what());
        return false;
    }
    return true;
}
