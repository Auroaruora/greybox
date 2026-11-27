#include "scenerenderer.h"

#include "gl/mesh.h"
#include "render/phongprogram.h"
#include "render/toonprogram.h"
#include "render/postprocess/gradepass.h"
#include "utils/shaderloader.h"

#include <string>
#include <vector>

static const std::vector<std::string> LutNames = {
    "2-Strip-Process",
    "Amber & Aqua",
    "Aqua",
    "BW1",
    "Cinematic-1",
    "Cinematic-Teal",
    "Golden Hour",
    "LC1",
    "Landscape1",
    "Moody1",
    "NW-1",
    "Portrait1",
};

void SceneRenderer::initialize() {
    m_programs[ShadingModel::Phong] = std::make_unique<PhongProgram>();
    m_programs[ShadingModel::Toon] = std::make_unique<ToonProgram>();
    for (auto &[shading, program] : m_programs) {
        program->initialize();
    }

    loadSky();
    m_postProcessor.initialize();
    for (const std::string &lut : LutNames) {
        m_postProcessor.addPass(std::make_unique<GradePass>(lut, "luts/" + lut + ".cube"));
    }
}

void SceneRenderer::setViewportSize(const glm::ivec2 &size) {
    m_viewportSize = size;
    m_postProcessor.setSize(size.x, size.y);
}

void SceneRenderer::render(const Camera &camera, const RenderData &data) {
    m_postProcessor.begin();
    drawScene(camera, data);
    m_postProcessor.end(m_viewportSize);
}

void SceneRenderer::loadSky() {
    ShaderLoader::load(m_skyShader, "shaders/fullscreen.vert", "shaders/sky.frag");
    m_skyUniforms.inverseViewProjection = m_skyShader.getLocation("inverseViewProjection");
    m_skyUniforms.cameraPos = m_skyShader.getLocation("cameraPos");
    m_skyUniforms.zenith = m_skyShader.getLocation("zenith");
    m_skyUniforms.horizon = m_skyShader.getLocation("horizon");
    m_skyQuad.create();
}

void SceneRenderer::drawSky(const Camera &camera, const RenderData &data) const {
    if (!data.sky || !m_skyShader.isValid()) {
        return;
    }
    glDisable(GL_DEPTH_TEST);
    m_skyShader.bind();
    m_skyShader.setMat4(m_skyUniforms.inverseViewProjection, glm::inverse(camera.getProjectionMatrix() * camera.getViewMatrix()));
    m_skyShader.setVec3(m_skyUniforms.cameraPos, camera.getPose().position);
    m_skyShader.setVec3(m_skyUniforms.zenith, data.sky->zenith);
    m_skyShader.setVec3(m_skyUniforms.horizon, data.sky->horizon);
    m_skyQuad.draw();
    m_skyShader.unbind();
}

void SceneRenderer::drawScene(const Camera &camera, const RenderData &data) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawSky(camera, data);

    ShadingProgram *program = getProgram(data.globalData.shading);
    if (program == nullptr || !program->isValid()) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    program->bind();
    program->setFrame(camera, data);
    for (const RenderShapeData &shape : data.shapes) {
        if (shape.mesh == nullptr || !shape.visible) {
            continue;
        }
        program->setShape(shape);
        shape.mesh->draw();
    }
    program->unbind();
}

ShadingProgram *SceneRenderer::getProgram(ShadingModel shading) {
    auto found = m_programs.find(shading);
    return found != m_programs.end() ? found->second.get() : nullptr;
}

void SceneRenderer::cleanup() {
    m_postProcessor.cleanup();
    m_skyQuad.cleanup();
    m_skyShader.cleanup();
    for (auto &[shading, program] : m_programs) {
        program->cleanup();
    }
}

PostProcessor &SceneRenderer::getPostProcessor() {
    return m_postProcessor;
}
