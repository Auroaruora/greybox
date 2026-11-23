#pragma once

#include "camera/camera.h"
#include "gl/shaderprogram.h"
#include "gl/texture2d.h"
#include "render/postprocess/postprocessor.h"
#include "utils/sceneparser.h"

#include <array>

class SceneRenderer {
public:
    void initialize();
    void setViewportSize(const glm::ivec2 &size);
    void render(const Camera &camera, const RenderData &data);
    void cleanup();

    PostProcessor &getPostProcessor();

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
        GLint ka = -1;
        GLint kd = -1;
        GLint ks = -1;
        GLint lightCount = -1;
        GLint modelMatrix = -1;
        GLint normalMatrix = -1;
        GLint ambientLight = -1;
        GLint cDiffuse = -1;
        GLint cSpecular = -1;
        GLint cEmissive = -1;
        GLint shininess = -1;
        GLint textureMap = -1;
        GLint textureBlend = -1;
        GLint textureRepeat = -1;
        GLint emissiveMap = -1;
        std::array<LightUniforms, MaxLights> lights;
    };

    void loadUniforms();
    void setCamera(const Camera &camera);
    void setGlobalData(const SceneGlobalData &globalData);
    void setLights(const std::vector<SceneLightData> &lights);
    void drawScene(const Camera &camera, const RenderData &data);
    void setTransform(const RenderShapeData &shape);
    void setMaterial(const RenderShapeData &shape);
    void setTextureMap(const RenderShapeData &shape);
    void setEmissiveMap(const RenderShapeData &shape);
    void bindTexture(const Texture2D &texture, int unit);

    ShaderProgram m_shader;
    Uniforms m_uniforms;
    Texture2D m_placeholderTexture;
    std::array<const Texture2D *, 2> m_boundTextures = {};
    PostProcessor m_postProcessor;
    glm::ivec2 m_viewportSize = glm::ivec2(0);
};
