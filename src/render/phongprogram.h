#pragma once

#include "render/shadingprogram.h"

class PhongProgram : public ShadingProgram {
public:
    PhongProgram();

protected:
    void loadUniforms() override;
    void setSceneData(const RenderData &data) override;
    void setMaterial(const RenderShapeData &shape) override;
    void setCamera(const Camera &camera) override;

private:
    struct Uniforms {
        GLint ka = -1;
        GLint kd = -1;
        GLint ks = -1;
        GLint ambientLight = -1;
        GLint cDiffuse = -1;
        GLint cSpecular = -1;
        GLint cEmissive = -1;
        GLint shininess = -1;
        GLint cameraPos = -1;
    };

    Uniforms m_uniforms;
};
