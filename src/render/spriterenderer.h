#pragma once

#include "gl/quad.h"
#include "gl/shaderprogram.h"
#include "gl/texture2d.h"

#include <glm/glm.hpp>

class SpriteRenderer {
public:
    void initialize();
    void cleanup();

    void draw(const Texture2D &texture, const glm::vec2 &center, const glm::vec2 &size, const glm::vec2 &viewport) const;

private:
    ShaderProgram m_shader;
    Quad m_quad;
    GLint m_rectLocation = -1;
};
