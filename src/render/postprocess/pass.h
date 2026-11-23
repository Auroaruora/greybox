#pragma once

#include "gl/framebuffer.h"
#include "gl/quad.h"
#include "gl/shaderprogram.h"

#include <string>

struct Pass {
    Pass(const std::string &name, const std::string &fragmentPath);
    virtual ~Pass() = default;
    virtual void initialize();
    virtual void render(const Texture2D &source, const Framebuffer &target, const Quad &quad) const;
    virtual void cleanup();

    std::string name;
    std::string fragmentPath;
    ShaderProgram shader;
    bool isEnabled = false;

protected:
    virtual void bindInputs() const;
};
