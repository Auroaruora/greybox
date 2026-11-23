#include "pass.h"

Pass::Pass(const std::string &name, const std::string &fragmentPath)
    : name(name), fragmentPath(fragmentPath) {
}

void Pass::initialize() {
}

void Pass::render(const Texture2D &source, const Framebuffer &target, const Quad &quad) const {
    target.bind();
    shader.bind();
    source.bind(0);
    bindInputs();
    quad.draw();
    source.unbind(0);
    shader.unbind();
}

void Pass::cleanup() {
    shader.cleanup();
}

void Pass::bindInputs() const {
}
