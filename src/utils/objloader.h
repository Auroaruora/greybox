#pragma once

#include <string>
#include <vector>

namespace ObjLoader {
bool load(const std::string &filepath, std::vector<float> &vertexData, std::string &material);
}