#pragma once

#include "render/shadingprogram.h"

class ToonProgram : public ShadingProgram {
public:
    ToonProgram();

protected:
    void loadUniforms() override;
    void setSceneData(const RenderData &data) override;
    void setMaterial(const RenderShapeData &shape) override;

private:
    struct Uniforms {
        GLint ambientLight = -1;
        GLint shadeColor = -1;
        GLint cDiffuse = -1;
        GLint cEmissive = -1;
    };

    Uniforms m_uniforms;
};
