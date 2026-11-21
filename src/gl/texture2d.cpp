#include "texture2d.h"

void Texture2D::create(int width, int height) {
    create(width, height, nullptr);
}

void Texture2D::create(int width, int height, const unsigned char *pixels, bool isSrgb) {
    cleanup();
    if (width <= 0 || height <= 0) {
        return;
    }
    m_width = width;
    m_height = height;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    GLint internalFormat = isSrgb ? GL_SRGB8_ALPHA8 : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture2D::unbind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::cleanup() {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
    m_width = 0;
    m_height = 0;
}

void Texture2D::generateMipmaps() const {
    if (m_id == 0) {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, m_id);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::setRepeat(bool isRepeat) const {
    if (m_id == 0) {
        return;
    }
    GLint mode = isRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool Texture2D::isValid() const {
    return m_id != 0;
}

GLuint Texture2D::getId() const {
    return m_id;
}

int Texture2D::getWidth() const {
    return m_width;
}

int Texture2D::getHeight() const {
    return m_height;
}
