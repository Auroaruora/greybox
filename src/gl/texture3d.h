#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>

class Texture3D {
public:
    Texture3D() = default;
    Texture3D(const Texture3D &) = delete;
    Texture3D &operator=(const Texture3D &) = delete;

    void create(int size, const float *values);
    void bind(int slot) const;
    void unbind(int slot) const;
    void cleanup();

    bool isValid() const;
    int getSize() const;

private:
    GLuint m_id = 0;
    int m_size = 0;
};
