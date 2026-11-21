#include "screenshot.h"

#include <GL/glew.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <filesystem>
#include <vector>

namespace Screenshot {

bool save(const std::string &path) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width = viewport[2];
    int height = viewport[3];

    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(viewport[0], viewport[1], width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    stbi_flip_vertically_on_write(1);
    return stbi_write_png(path.c_str(), width, height, 3, pixels.data(), width * 3) != 0;
}

}
