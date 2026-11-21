#include "scenerenderer.h"

#include "gl/mesh.h"
#include "gl/texture2d.h"
#include "utils/shaderloader.h"

#include <string>

static const int TextureMapUnit = 0;
static const int EmissiveMapUnit = 1;
static const unsigned char PlaceholderPixel[4] = {255, 255, 255, 255};

void SceneRenderer::initialize() {
    ShaderLoader::load(m_shader, "shaders/default.vert", "shaders/default.frag");
    loadUniforms();
    m_placeholderTexture.create(1, 1, PlaceholderPixel);
}

void SceneRenderer::loadUniforms() {
    m_uniforms.viewMatrix = m_shader.getLocation("viewMatrix");
    m_uniforms.projMatrix = m_shader.getLocation("projMatrix");
    m_uniforms.cameraPos = m_shader.getLocation("cameraPos");
    m_uniforms.ka = m_shader.getLocation("ka");
    m_uniforms.kd = m_shader.getLocation("kd");
    m_uniforms.ks = m_shader.getLocation("ks");
    m_uniforms.lightCount = m_shader.getLocation("lightCount");
    m_uniforms.modelMatrix = m_shader.getLocation("modelMatrix");
    m_uniforms.normalMatrix = m_shader.getLocation("normalMatrix");
    m_uniforms.ambientLight = m_shader.getLocation("ambientLight");
    m_uniforms.cDiffuse = m_shader.getLocation("cDiffuse");
    m_uniforms.cSpecular = m_shader.getLocation("cSpecular");
    m_uniforms.cEmissive = m_shader.getLocation("cEmissive");
    m_uniforms.shininess = m_shader.getLocation("shininess");
    m_uniforms.textureMap = m_shader.getLocation("textureMap");
    m_uniforms.textureBlend = m_shader.getLocation("textureBlend");
    m_uniforms.textureRepeat = m_shader.getLocation("textureRepeat");
    m_uniforms.emissiveMap = m_shader.getLocation("emissiveMap");
    m_shader.bind();
    m_shader.setInt(m_uniforms.textureMap, TextureMapUnit);
    m_shader.setInt(m_uniforms.emissiveMap, EmissiveMapUnit);
    m_shader.unbind();

    for (int i = 0; i < MaxLights; i++) {
        std::string prefix = "lights[" + std::to_string(i) + "].";
        LightUniforms &light = m_uniforms.lights[i];
        light.type = m_shader.getLocation(prefix + "type");
        light.color = m_shader.getLocation(prefix + "color");
        light.pos = m_shader.getLocation(prefix + "pos");
        light.dir = m_shader.getLocation(prefix + "dir");
        light.function = m_shader.getLocation(prefix + "function");
        light.angle = m_shader.getLocation(prefix + "angle");
        light.penumbra = m_shader.getLocation(prefix + "penumbra");
    }
}

void SceneRenderer::setCamera(const Camera &camera) {
    m_shader.setMat4(m_uniforms.viewMatrix, camera.getViewMatrix());
    m_shader.setMat4(m_uniforms.projMatrix, camera.getProjectionMatrix());
    m_shader.setVec3(m_uniforms.cameraPos, camera.getPose().position);
}

void SceneRenderer::setGlobalData(const SceneGlobalData &globalData) {
    m_shader.setFloat(m_uniforms.ka, globalData.ka);
    m_shader.setFloat(m_uniforms.kd, globalData.kd);
    m_shader.setFloat(m_uniforms.ks, globalData.ks);
    m_shader.setVec3(m_uniforms.ambientLight, globalData.ambientLight);
}

void SceneRenderer::setLights(const std::vector<SceneLightData> &lights) {
    int count = 0;
    for (const SceneLightData &light : lights) {
        if (!light.enabled) {
            continue;
        }
        if (count == MaxLights) {
            break;
        }
        const LightUniforms &uniforms = m_uniforms.lights[count];
        m_shader.setInt(uniforms.type, static_cast<int>(light.type));
        m_shader.setVec3(uniforms.color, glm::vec3(light.color));
        m_shader.setVec3(uniforms.pos, glm::vec3(light.pos));
        m_shader.setVec3(uniforms.dir, glm::vec3(light.dir));
        m_shader.setVec3(uniforms.function, light.function);
        m_shader.setFloat(uniforms.angle, light.angle);
        m_shader.setFloat(uniforms.penumbra, light.penumbra);
        count++;
    }
    m_shader.setInt(m_uniforms.lightCount, count);
}

void SceneRenderer::render(const Camera &camera, const RenderData &data) {
    drawScene(camera, data);
}

void SceneRenderer::drawScene(const Camera &camera, const RenderData &data) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_shader.isValid()) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    m_shader.bind();
    setCamera(camera);
    setGlobalData(data.globalData);
    setLights(data.lights);

    m_boundTextures.fill(nullptr);
    for (const RenderShapeData &shape : data.shapes) {
        if (shape.mesh == nullptr || !shape.visible) {
            continue;
        }
        setTransform(shape);
        setMaterial(shape);
        setTextureMap(shape);
        setEmissiveMap(shape);
        shape.mesh->draw();
    }
    m_shader.unbind();
}

void SceneRenderer::setTransform(const RenderShapeData &shape) {
    m_shader.setMat4(m_uniforms.modelMatrix, shape.ctm);
    m_shader.setMat3(m_uniforms.normalMatrix, shape.normalMatrix);
}

void SceneRenderer::setMaterial(const RenderShapeData &shape) {
    const SceneMaterial &material = shape.primitive.material;
    m_shader.setVec3(m_uniforms.cDiffuse, glm::vec3(material.cDiffuse));
    m_shader.setVec3(m_uniforms.cSpecular, glm::vec3(material.cSpecular));
    m_shader.setVec3(m_uniforms.cEmissive, shape.emissiveEnabled ? glm::vec3(material.cEmissive) : glm::vec3(0.0f));
    m_shader.setFloat(m_uniforms.shininess, material.shininess);
}

void SceneRenderer::setTextureMap(const RenderShapeData &shape) {
    if (shape.textureMap == nullptr) {
        m_shader.setFloat(m_uniforms.textureBlend, 0.0f);
        m_shader.setVec2(m_uniforms.textureRepeat, glm::vec2(1.0f));
        bindTexture(m_placeholderTexture, TextureMapUnit);
        return;
    }
    const SceneMaterial &material = shape.primitive.material;
    m_shader.setFloat(m_uniforms.textureBlend, material.blend);
    m_shader.setVec2(m_uniforms.textureRepeat, glm::vec2(material.textureMap.repeatU, material.textureMap.repeatV));
    bindTexture(*shape.textureMap, TextureMapUnit);
}

void SceneRenderer::setEmissiveMap(const RenderShapeData &shape) {
    if (shape.emissiveMap == nullptr || !shape.emissiveEnabled) {
        bindTexture(m_placeholderTexture, EmissiveMapUnit);
        return;
    }
    bindTexture(*shape.emissiveMap, EmissiveMapUnit);
}

void SceneRenderer::bindTexture(const Texture2D &texture, int unit) {
    if (m_boundTextures[unit] == &texture) {
        return;
    }
    texture.bind(unit);
    m_boundTextures[unit] = &texture;
}

void SceneRenderer::cleanup() {
    m_placeholderTexture.cleanup();
    m_shader.cleanup();
}
