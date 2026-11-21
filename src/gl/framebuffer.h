#pragma once

#include "gl/texture2d.h"

class Framebuffer {
public:
    Framebuffer() = default;
    Framebuffer(const Framebuffer &) = delete;
    Framebuffer &operator=(const Framebuffer &) = delete;

    void create(int width, int height, bool hasDepth = true);
    void bind() const;
    void unbind() const;
    void cleanup();

    bool isValid() const;
    int getWidth() const;
    int getHeight() const;
    const Texture2D &getTexture() const;

private:
    GLuint m_fbo = 0;
    Texture2D m_texture;
    GLuint m_renderbuffer = 0;
};
