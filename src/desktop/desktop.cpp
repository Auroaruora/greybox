#include "desktop.h"

#include "render/spriterenderer.h"
#include "utils/imageloader.h"

#include <glm/glm.hpp>

#include <string>

static const std::string WallpaperFile = "ui/desktop/wallpapers/mountains.png";
static const glm::vec4 Background(0.10f, 0.13f, 0.18f, 1.0f);

void Desktop::initialize() {
    m_framebuffer.create(Width, Height, false);
    Image image;
    if (ImageLoader::load(WallpaperFile, image)) {
        m_wallpaper.create(image.width, image.height, image.pixels.data());
    }
    m_isDirty = true;
}

void Desktop::updateScreenTexture(const SpriteRenderer &sprites) {
    if (!m_isDirty || !m_framebuffer.isValid()) {
        return;
    }
    m_framebuffer.bind();
    glClearColor(Background.r, Background.g, Background.b, Background.a);
    glClear(GL_COLOR_BUFFER_BIT);
    glm::vec2 size(Width, Height);
    sprites.draw(m_wallpaper, size * 0.5f, size, size);
    m_framebuffer.unbind();
    m_framebuffer.getTexture().generateMipmaps();
    m_isDirty = false;
}

void Desktop::cleanup() {
    m_wallpaper.cleanup();
    m_framebuffer.cleanup();
}

const Texture2D &Desktop::getScreenTexture() const {
    return m_framebuffer.getTexture();
}
