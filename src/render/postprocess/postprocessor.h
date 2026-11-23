#pragma once

#include "gl/framebuffer.h"
#include "gl/quad.h"
#include "gl/shaderprogram.h"
#include "render/postprocess/pass.h"

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <string>
#include <vector>

class PostProcessor {
public:
    void initialize();
    void addPass(std::unique_ptr<Pass> pass);
    void movePass(int from, int to);
    void setSize(int width, int height);
    void begin() const;
    void end(const glm::ivec2 &screenSize) const;
    void cleanup();

    const std::vector<std::unique_ptr<Pass>> &getPasses() const;

private:
    void loadShader(ShaderProgram &shader, const std::string &fragmentPath) const;
    void paintTexture(const Texture2D &texture, const ShaderProgram &shader) const;

    std::array<Framebuffer, 2> m_buffers;
    Quad m_fullscreenQuad;
    ShaderProgram m_copyShader;
    std::vector<std::unique_ptr<Pass>> m_passes;
};
