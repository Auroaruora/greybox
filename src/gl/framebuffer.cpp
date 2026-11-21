#include "framebuffer.h"

#include "debug/debug.h"

void Framebuffer::create(int width, int height, bool hasDepth) {
    cleanup();
    m_texture.create(width, height);
    if (!m_texture.isValid()) {
        return;
    }

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture.getId(), 0);

    if (hasDepth) {
        glGenRenderbuffers(1, &m_renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_renderbuffer);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("framebuffer is not complete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_texture.getWidth(), m_texture.getHeight());
}

void Framebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::cleanup() {
    m_texture.cleanup();
    if (m_renderbuffer != 0) {
        glDeleteRenderbuffers(1, &m_renderbuffer);
        m_renderbuffer = 0;
    }
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
}

bool Framebuffer::isValid() const {
    return m_fbo != 0;
}

int Framebuffer::getWidth() const {
    return m_texture.getWidth();
}

int Framebuffer::getHeight() const {
    return m_texture.getHeight();
}

const Texture2D &Framebuffer::getTexture() const {
    return m_texture;
}
