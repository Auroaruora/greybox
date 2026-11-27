#include "toonprogram.h"

static const std::string FragmentPath = "shaders/toon.frag";
static const glm::vec3 DefaultShade(0.45f, 0.5f, 0.7f);

ToonProgram::ToonProgram() : ShadingProgram(FragmentPath) {
}

void ToonProgram::loadUniforms() {
    m_uniforms.ambientLight = m_shader.getLocation("ambientLight");
    m_uniforms.shadeColor = m_shader.getLocation("shadeColor");
    m_uniforms.cDiffuse = m_shader.getLocation("cDiffuse");
    m_uniforms.cEmissive = m_shader.getLocation("cEmissive");
}

void ToonProgram::setSceneData(const RenderData &data) {
    m_shader.setVec3(m_uniforms.ambientLight, data.globalData.ambientLight);
    m_shader.setVec3(m_uniforms.shadeColor, data.toonShade.value_or(DefaultShade));
}

void ToonProgram::setMaterial(const RenderShapeData &shape) {
    const SceneMaterial &material = shape.primitive.material;
    m_shader.setVec3(m_uniforms.cDiffuse, glm::vec3(material.cDiffuse));
    m_shader.setVec3(m_uniforms.cEmissive, shape.emissiveEnabled ? glm::vec3(material.cEmissive) : glm::vec3(0.0f));
}
