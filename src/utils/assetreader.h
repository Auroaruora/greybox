#pragma once

#include <glm/glm.hpp>

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct ColourSet {
    std::vector<std::string> shapes;
    std::map<std::string, std::vector<std::string>> parts;
};

struct AssetDoc {
    std::vector<std::string> parts;
    std::vector<std::string> shapes;
    std::vector<std::pair<std::string, ColourSet>> colours;
    std::map<std::string, std::string> textures;
};

struct PaletteColour {
    glm::dvec3 linear = glm::dvec3(0.5);
    std::optional<glm::dvec3> emission;
};

using Palette = std::map<std::string, PaletteColour>;

namespace AssetReader {
bool load(const std::string &asset, AssetDoc &doc);
bool loadPalette(Palette &palette);
}
