#pragma once

#include <string>
#include <vector>

struct Lut {
    int size = 0;
    std::vector<float> values;
};

namespace LutLoader {
bool load(const std::string &path, Lut &lut);
}
