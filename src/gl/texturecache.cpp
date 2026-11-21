#include "texturecache.h"

#include "utils/imageloader.h"

const Texture2D *TextureCache::getTexture(const std::string &filepath) {
    auto found = m_textures.find(filepath);
    if (found != m_textures.end()) {
        return &found->second;
    }
    if (m_failed.contains(filepath)) {
        return nullptr;
    }

    Image image;
    if (!ImageLoader::load(filepath, image)) {
        m_failed.insert(filepath);
        return nullptr;
    }

    Texture2D &texture = m_textures[filepath];
    texture.create(image.width, image.height, image.pixels.data(), true);
    texture.generateMipmaps();
    texture.setRepeat(true);
    return &texture;
}

void TextureCache::clear() {
    for (auto &[filepath, texture] : m_textures) {
        texture.cleanup();
    }
    m_textures.clear();
    m_failed.clear();
}
