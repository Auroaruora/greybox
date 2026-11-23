#include "gradepass.h"

#include "utils/lutloader.h"

GradePass::GradePass(const std::string &name, const std::string &lutPath)
    : Pass(name, "shaders/grade.frag"), m_lutPath(lutPath) {
}

void GradePass::initialize() {
    Lut lut;
    if (!LutLoader::load(m_lutPath, lut)) {
        shader.cleanup();
        return;
    }
    m_lut.create(lut.size, lut.values.data());

    shader.bind();
    shader.setInt(shader.getLocation("lut"), 1);
    shader.setFloat(shader.getLocation("lutSize"), static_cast<float>(lut.size));
    shader.unbind();
}

void GradePass::bindInputs() const {
    m_lut.bind(1);
}

void GradePass::cleanup() {
    m_lut.cleanup();
    Pass::cleanup();
}
