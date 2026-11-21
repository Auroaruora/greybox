#include "spriterenderer.h"

#include "utils/shaderloader.h"

void SpriteRenderer::initialize() {
    ShaderLoader::load(m_shader, "shaders/sprite.vert", "shaders/sprite.frag");
    m_rectLocation = m_shader.getLocation("rect");
    m_shader.bind();
    m_shader.setInt(m_shader.getLocation("spriteTexture"), 0);
    m_shader.unbind();
    m_quad.create();
}

void SpriteRenderer::cleanup() {
    m_quad.cleanup();
    m_shader.cleanup();
}

void SpriteRenderer::draw(const Texture2D &texture, const glm::vec2 &center, const glm::vec2 &size, const glm::vec2 &viewport) const {
    if (!m_shader.isValid() || !texture.isValid()) {
        return;
    }

    glm::vec2 ndcCenter(center.x / viewport.x * 2.0f - 1.0f, 1.0f - center.y / viewport.y * 2.0f);
    glm::vec2 ndcHalfSize = size / viewport;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.bind();
    m_shader.setVec4(m_rectLocation, glm::vec4(ndcCenter, ndcHalfSize));
    texture.bind(0);

    m_quad.draw();

    texture.unbind(0);
    m_shader.unbind();
}
