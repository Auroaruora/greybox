#include "imageloader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool ImageLoader::load(const std::string &path, Image &image) {
    stbi_set_flip_vertically_on_load(1);
    int channels = 0;
    unsigned char *pixels = stbi_load(Paths::getAssetPath(path).c_str(), &image.width, &image.height, &channels, 4);
    if (pixels == nullptr) {
        LOG_ERROR("cannot load " << path);
        return false;
    }

    image.pixels.assign(pixels, pixels + static_cast<size_t>(image.width) * image.height * 4);
    stbi_image_free(pixels);
    return true;
}
