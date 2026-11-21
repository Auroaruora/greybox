#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>

#include <vector>

class Mesh {
public:
    static constexpr int FloatsPerVertex = 8;

    Mesh() = default;
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    void create(const std::vector<float> &vertexData);
    void draw() const;
    void cleanup();

private:
    GLuint m_vbo = 0;
    GLuint m_vao = 0;
    GLsizei m_vertexCount = 0;
};