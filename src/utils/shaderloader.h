#pragma once

#include "gl/shaderprogram.h"

#include <string>

namespace ShaderLoader {
bool load(ShaderProgram &shader, const std::string &vertexFile, const std::string &fragmentFile);
}
