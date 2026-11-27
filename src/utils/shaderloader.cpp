#include "shaderloader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <filesystem>
#include <fstream>
#include <sstream>

static const std::string IncludePrefix = "#include \"";

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

static bool expandIncludes(const std::string &filepath, std::string &source) {
    std::string folder = std::filesystem::path(filepath).parent_path().string();
    std::istringstream lines(source);
    std::ostringstream expanded;
    std::string line;
    while (std::getline(lines, line)) {
        if (line.rfind(IncludePrefix, 0) != 0) {
            expanded << line << '\n';
            continue;
        }
        std::string name = line.substr(IncludePrefix.size(), line.find('"', IncludePrefix.size()) - IncludePrefix.size());
        std::string included;
        if (!loadFile(folder + "/" + name, included)) {
            return false;
        }
        expanded << included << '\n';
    }
    source = expanded.str();
    return true;
}

static bool loadSource(const std::string &file, std::string &source) {
    std::string filepath = Paths::getAssetPath(file);
    return loadFile(filepath, source) && expandIncludes(filepath, source);
}

bool ShaderLoader::load(ShaderProgram &shader, const std::string &vertexFile, const std::string &fragmentFile) {
    std::string vertex;
    std::string fragment;
    if (!loadSource(vertexFile, vertex) || !loadSource(fragmentFile, fragment)) {
        return false;
    }
    shader.setSourceFiles(vertexFile, fragmentFile);
    if (!shader.create(vertex, fragment)) {
        LOG_ERROR("shader failed: " << vertexFile << " + " << fragmentFile);
        return false;
    }
    return true;
}
