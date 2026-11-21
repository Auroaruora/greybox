#include "shaderloader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <fstream>
#include <sstream>

static bool loadFile(const std::string &filepath, std::string &source) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("cannot open " << filepath);
        return false;
    }
    std::stringstream stream;
    stream << file.rdbuf();
    source = stream.str();
    return true;
}

bool ShaderLoader::load(ShaderProgram &shader, const std::string &vertexFile, const std::string &fragmentFile) {
    std::string vertex;
    std::string fragment;
    if (!loadFile(Paths::getAssetPath(vertexFile), vertex) || !loadFile(Paths::getAssetPath(fragmentFile), fragment)) {
        return false;
    }
    shader.setSourceFiles(vertexFile, fragmentFile);
    if (!shader.create(vertex, fragment)) {
        LOG_ERROR("shader failed: " << vertexFile << " + " << fragmentFile);
        return false;
    }
    return true;
}
