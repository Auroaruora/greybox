#pragma once

#include <string>

namespace Paths {
std::string getAssetPath(const std::string &relativePath);
std::string getCapturePath(const std::string &filename);
}