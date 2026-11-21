#pragma once

#include <string>
#include <vector>

struct Image {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

namespace ImageLoader {
bool load(const std::string &path, Image &image);
}
