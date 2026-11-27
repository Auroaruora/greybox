#include "assetreader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

using json = nlohmann::ordered_json;

static const std::string PaletteFile = "materials/palette.json";

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

static std::vector<std::string> readNames(const json &value) {
    std::vector<std::string> names;
    for (const json &name : value) {
        names.push_back(name.get<std::string>());
    }
    return names;
}

static ColourSet readColourSet(const json &data, const std::vector<std::string> &shapes) {
    ColourSet set;
    set.shapes = data.contains("shapes") ? readNames(data.at("shapes")) : shapes;
    for (const auto &[part, value] : data.items()) {
        if (part == "shapes") {
            continue;
        }
        set.parts[part] = value.is_array() ? readNames(value) : std::vector<std::string>{value.get<std::string>()};
    }
    return set;
}

bool AssetReader::load(const std::string &asset, AssetDoc &doc) {
    std::string path = "models/" + asset + "/asset.json";
    try {
        json data = readJson(path);
        doc.parts = readNames(data.at("parts"));
        doc.shapes = readNames(data.at("shapes"));
        for (const auto &[name, set] : data.at("colors").items()) {
            doc.colours.push_back({name, readColourSet(set, doc.shapes)});
        }
        if (data.contains("materials")) {
            for (const auto &[name, material] : data.at("materials").items()) {
                if (material.contains("color")) {
                    doc.textures[name] = "models/" + asset + "/" + material.at("color").get<std::string>();
                }
            }
        }
    } catch (const std::exception &error) {
        LOG_ERROR(path << ": " << error.what());
        return false;
    }
    return true;
}

bool AssetReader::loadPalette(Palette &palette) {
    try {
        json data = readJson(PaletteFile);
        for (const auto &[name, entry] : data.items()) {
            PaletteColour colour;
            colour.linear = readVec3(entry.at("linear"));
            if (entry.contains("emission")) {
                colour.emission = readVec3(entry.at("emission"));
            }
            palette[name] = colour;
        }
    } catch (const std::exception &error) {
        LOG_ERROR(PaletteFile << ": " << error.what());
        return false;
    }
    return true;
}
