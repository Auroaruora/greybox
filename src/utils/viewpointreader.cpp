#include "viewpointreader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <nlohmann/json.hpp>

#include <fstream>

using json = nlohmann::json;

static glm::vec3 readVec3(const json &value) {
    return glm::vec3(value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>());
}

ViewpointReader::ViewpointReader(const std::string &filename) {
    m_filename = filename;
}

bool ViewpointReader::readJSON() {
    std::ifstream file(Paths::getAssetPath(m_filename));
    if (!file.is_open()) {
        LOG_ERROR("cannot open " << m_filename);
        return false;
    }

    try {
        json data = json::parse(file);
        for (const auto &[name, camera] : data.items()) {
            Viewpoint viewpoint;
            viewpoint.name = name;
            viewpoint.pose.position = readVec3(camera.at("position"));
            viewpoint.pose.look = readVec3(camera.at("look"));
            viewpoint.pose.up = readVec3(camera.at("up"));
            viewpoint.pose.heightAngle = glm::radians(camera.at("heightAngle").get<float>());
            m_viewpoints.push_back(viewpoint);
        }
    } catch (const std::exception &error) {
        LOG_ERROR(m_filename << ": " << error.what());
        m_viewpoints.clear();
        return false;
    }
    return true;
}

const std::vector<Viewpoint> &ViewpointReader::getViewpoints() const {
    return m_viewpoints;
}
