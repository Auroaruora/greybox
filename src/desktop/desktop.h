#pragma once

#include "gl/framebuffer.h"
#include "gl/texture2d.h"

class SpriteRenderer;

class Desktop {
public:
    static constexpr int Width = 1280;
    static constexpr int Height = 760;

    void initialize();
    void updateScreenTexture(const SpriteRenderer &sprites);
    void cleanup();

    const Texture2D &getScreenTexture() const;

private:
    Framebuffer m_framebuffer;
    Texture2D m_wallpaper;
    bool m_isDirty = true;
};
