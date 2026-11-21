#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>

class Texture2D {
public:
    Texture2D() = default;
    Texture2D(const Texture2D &) = delete;
    Texture2D &operator=(const Texture2D &) = delete;

    void create(int width, int height);
    void create(int width, int height, const unsigned char *pixels, bool isSrgb = false);
    void bind(int slot) const;
    void unbind(int slot) const;
    void generateMipmaps() const;
    void setRepeat(bool isRepeat) const;
    void cleanup();

    bool isValid() const;
    GLuint getId() const;
    int getWidth() const;
    int getHeight() const;

private:
    GLuint m_id = 0;
    int m_width = 0;
    int m_height = 0;
};
