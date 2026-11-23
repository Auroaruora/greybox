#include "lutloader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <fstream>
#include <sstream>

bool LutLoader::load(const std::string &path, Lut &lut) {
    std::ifstream file(Paths::getAssetPath(path));
    if (!file.is_open()) {
        LOG_ERROR("cannot open " << path);
        return false;
    }

    lut = Lut();
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        if (line.starts_with("LUT_3D_SIZE")) {
            std::string keyword;
            stream >> keyword >> lut.size;
            continue;
        }
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        if (stream >> r >> g >> b) {
            lut.values.insert(lut.values.end(), {r, g, b});
        }
    }

    size_t expected = static_cast<size_t>(lut.size) * lut.size * lut.size * 3;
    if (lut.size <= 0 || lut.values.size() != expected) {
        LOG_ERROR(path << ": not a valid .cube file");
        return false;
    }
    return true;
}
