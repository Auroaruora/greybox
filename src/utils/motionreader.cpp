#include "motionreader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

using json = nlohmann::ordered_json;

static glm::vec3 readVec3(const json &value) {
    return glm::vec3(value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>());
}

bool MotionReader::loadMotions(const std::string &file, std::vector<MotionSource> &motions) {
    std::string path = Paths::getAssetPath(file);
    if (!std::filesystem::exists(path)) {
        return true;
    }
    try {
        std::ifstream stream(path);
        json data = json::parse(stream);
        for (const auto &[target, entry] : data.items()) {
            MotionSource motion;
            size_t split = target.find('/');
            motion.object = target.substr(0, split);
            motion.part = split == std::string::npos ? "" : target.substr(split + 1);
            motion.pivot = entry.at("pivot").get<std::string>();
            if (entry.contains("spin")) {
                motion.spin = readVec3(entry.at("spin"));
            }
            if (entry.contains("sway")) {
                motion.sway = readVec3(entry.at("sway"));
            }
            motion.period = entry.value("period", 1.0f);
            motions.push_back(motion);
        }
    } catch (const std::exception &error) {
        LOG_ERROR(file << ": " << error.what());
        return false;
    }
    return true;
}

bool MotionReader::loadPath(const std::string &file, std::vector<PathKeySource> &keys) {
    try {
        std::ifstream stream(Paths::getAssetPath(file));
        json data = json::parse(stream);
        for (const json &entry : data) {
            PathKeySource key;
            key.time = entry.at("time").get<float>();
            key.position = readVec3(entry.at("position"));
            key.look = readVec3(entry.at("look"));
            key.up = readVec3(entry.at("up"));
            key.heightAngle = glm::radians(entry.at("heightAngle").get<float>());
            keys.push_back(key);
        }
    } catch (const std::exception &error) {
        LOG_ERROR(file << ": " << error.what());
        return false;
    }
    return true;
}
