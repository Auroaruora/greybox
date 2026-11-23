#include "texture3d.h"

void Texture3D::create(int size, const float *values) {
    cleanup();
    if (size <= 0) {
        return;
    }
    m_size = size;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_3D, m_id);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F, m_size, m_size, m_size, 0, GL_RGB, GL_FLOAT, values);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture3D::bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, m_id);
}

void Texture3D::unbind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture3D::cleanup() {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
    m_size = 0;
}

bool Texture3D::isValid() const {
    return m_id != 0;
}

int Texture3D::getSize() const {
    return m_size;
}
