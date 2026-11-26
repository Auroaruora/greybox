#include "phongprogram.h"

static const std::string FragmentPath = "shaders/phong.frag";

PhongProgram::PhongProgram() : ShadingProgram(FragmentPath) {
}

void PhongProgram::loadUniforms() {
    m_uniforms.ka = m_shader.getLocation("ka");
    m_uniforms.kd = m_shader.getLocation("kd");
    m_uniforms.ks = m_shader.getLocation("ks");
    m_uniforms.ambientLight = m_shader.getLocation("ambientLight");
    m_uniforms.cDiffuse = m_shader.getLocation("cDiffuse");
    m_uniforms.cSpecular = m_shader.getLocation("cSpecular");
    m_uniforms.cEmissive = m_shader.getLocation("cEmissive");
    m_uniforms.shininess = m_shader.getLocation("shininess");
}

void PhongProgram::setGlobalData(const SceneGlobalData &globalData) {
    m_shader.setFloat(m_uniforms.ka, globalData.ka);
    m_shader.setFloat(m_uniforms.kd, globalData.kd);
    m_shader.setFloat(m_uniforms.ks, globalData.ks);
    m_shader.setVec3(m_uniforms.ambientLight, globalData.ambientLight);
}

void PhongProgram::setMaterial(const RenderShapeData &shape) {
    const SceneMaterial &material = shape.primitive.material;
    m_shader.setVec3(m_uniforms.cDiffuse, glm::vec3(material.cDiffuse));
    m_shader.setVec3(m_uniforms.cSpecular, glm::vec3(material.cSpecular));
    m_shader.setVec3(m_uniforms.cEmissive, shape.emissiveEnabled ? glm::vec3(material.cEmissive) : glm::vec3(0.0f));
    m_shader.setFloat(m_uniforms.shininess, material.shininess);
}
