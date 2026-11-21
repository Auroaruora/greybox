#pragma once

#include "gl/texture2d.h"

#include <map>
#include <set>
#include <string>

class TextureCache {
public:
    const Texture2D *getTexture(const std::string &filepath);
    void clear();

private:
    std::map<std::string, Texture2D> m_textures;
    std::set<std::string> m_failed;
};
