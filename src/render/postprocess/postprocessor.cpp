#include "postprocessor.h"

#include "utils/shaderloader.h"

#include <utility>

void PostProcessor::initialize() {
    m_fullscreenQuad.create();
    loadShader(m_copyShader, "shaders/copy.frag");
}

void PostProcessor::loadShader(ShaderProgram &shader, const std::string &fragmentPath) const {
    ShaderLoader::load(shader, "shaders/fullscreen.vert", fragmentPath);
    shader.bind();
    shader.setInt(shader.getLocation("sourceTexture"), 0);
    shader.unbind();
}

void PostProcessor::addPass(std::unique_ptr<Pass> pass) {
    loadShader(pass->shader, pass->fragmentPath);
    pass->initialize();
    m_passes.push_back(std::move(pass));
}

void PostProcessor::movePass(int from, int to) {
    int count = static_cast<int>(m_passes.size());
    if (from < 0 || from >= count || to < 0 || to >= count) {
        return;
    }
    std::swap(m_passes[from], m_passes[to]);
}

void PostProcessor::setSize(int width, int height) {
    if (width == m_buffers[0].getWidth() && height == m_buffers[0].getHeight()) {
        return;
    }
    m_buffers[0].create(width, height, true);
    m_buffers[1].create(width, height, false);
}

void PostProcessor::begin() const {
    m_buffers[0].bind();
}

void PostProcessor::end(const glm::ivec2 &screenSize) const {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    int source = 0;
    for (const std::unique_ptr<Pass> &pass : m_passes) {
        if (!pass->isEnabled || !pass->shader.isValid()) {
            continue;
        }
        int target = 1 - source;
        pass->render(m_buffers[source].getTexture(), m_buffers[target], m_fullscreenQuad);
        source = target;
    }

    m_buffers[source].unbind();
    glViewport(0, 0, screenSize.x, screenSize.y);
    paintTexture(m_buffers[source].getTexture(), m_copyShader);
}

void PostProcessor::paintTexture(const Texture2D &texture, const ShaderProgram &shader) const {
    shader.bind();
    texture.bind(0);
    m_fullscreenQuad.draw();
    texture.unbind(0);
    shader.unbind();
}

void PostProcessor::cleanup() {
    for (std::unique_ptr<Pass> &pass : m_passes) {
        pass->cleanup();
    }
    m_passes.clear();

    m_copyShader.cleanup();
    m_fullscreenQuad.cleanup();
    for (Framebuffer &buffer : m_buffers) {
        buffer.cleanup();
    }
}

const std::vector<std::unique_ptr<Pass>> &PostProcessor::getPasses() const {
    return m_passes;
}
