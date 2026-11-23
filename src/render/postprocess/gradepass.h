#pragma once

#include "gl/texture3d.h"
#include "render/postprocess/pass.h"

#include <string>

class GradePass : public Pass {
public:
    GradePass(const std::string &name, const std::string &lutPath);

    void initialize() override;
    void cleanup() override;

protected:
    void bindInputs() const override;

private:
    std::string m_lutPath;
    Texture3D m_lut;
};
