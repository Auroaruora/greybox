#pragma once

#include "camera/camera.h"
#include "gl/shaderprogram.h"
#include "gl/texture2d.h"
#include "utils/sceneparser.h"

#include <array>
#include <string>
#include <vector>

class ShadingProgram {
public:
    explicit ShadingProgram(const std::string &fragmentPath);
    virtual ~ShadingProgram() = default;

    void initialize();
    void cleanup();
    void bind() const;
    void unbind() const;
    bool isValid() const;

    void setFrame(const Camera &camera, const RenderData &data);
    void setShape(const RenderShapeData &shape);

protected:
    virtual void loadUniforms() = 0;
    virtual void setGlobalData(const SceneGlobalData &globalData) = 0;
    virtual void setMaterial(const RenderShapeData &shape) = 0;

    virtual void setCamera(const Camera &camera);
    virtual void setLights(const std::vector<SceneLightData> &lights);
    virtual void setTransform(const RenderShapeData &shape);
    virtual void setTextureMap(const RenderShapeData &shape);
    virtual void setEmissiveMap(const RenderShapeData &shape);
    void bindTexture(const Texture2D &texture, int unit);

    ShaderProgram m_shader;

private:
    static constexpr int MaxLights = 8;

    struct LightUniforms {
        GLint type = -1;
        GLint color = -1;
        GLint pos = -1;
        GLint dir = -1;
        GLint function = -1;
        GLint angle = -1;
        GLint penumbra = -1;
    };

    struct Uniforms {
        GLint viewMatrix = -1;
        GLint projMatrix = -1;
        GLint cameraPos = -1;
        GLint modelMatrix = -1;
        GLint normalMatrix = -1;
        GLint lightCount = -1;
        GLint textureMap = -1;
        GLint textureBlend = -1;
        GLint textureRepeat = -1;
        GLint emissiveMap = -1;
        std::array<LightUniforms, MaxLights> lights;
    };

    void loadSharedUniforms();

    std::string m_fragmentPath;
    Uniforms m_uniforms;
    Texture2D m_placeholderTexture;
    std::array<const Texture2D *, 2> m_boundTextures = {};
};
