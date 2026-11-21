#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>

class Quad {
public:
    Quad() = default;
    Quad(const Quad &) = delete;
    Quad &operator=(const Quad &) = delete;

    void create();
    void draw() const;
    void cleanup();

private:
    GLuint m_vbo = 0;
    GLuint m_vao = 0;
};
