#include "shadingprogram.h"

#include "utils/shaderloader.h"

static const std::string VertexPath = "shaders/mesh.vert";
static const int TextureMapUnit = 0;
static const int EmissiveMapUnit = 1;
static const unsigned char PlaceholderPixel[4] = {255, 255, 255, 255};

ShadingProgram::ShadingProgram(const std::string &fragmentPath) {
    m_fragmentPath = fragmentPath;
}

void ShadingProgram::initialize() {
    ShaderLoader::load(m_shader, VertexPath, m_fragmentPath);
    loadSharedUniforms();
    loadUniforms();
    m_placeholderTexture.create(1, 1, PlaceholderPixel);
}

void ShadingProgram::cleanup() {
    m_placeholderTexture.cleanup();
    m_shader.cleanup();
}

void ShadingProgram::bind() const {
    m_shader.bind();
}

void ShadingProgram::unbind() const {
    m_shader.unbind();
}

bool ShadingProgram::isValid() const {
    return m_shader.isValid();
}

void ShadingProgram::loadSharedUniforms() {
    m_uniforms.viewMatrix = m_shader.getLocation("viewMatrix");
    m_uniforms.projMatrix = m_shader.getLocation("projMatrix");
    m_uniforms.cameraPos = m_shader.getLocation("cameraPos");
    m_uniforms.modelMatrix = m_shader.getLocation("modelMatrix");
    m_uniforms.normalMatrix = m_shader.getLocation("normalMatrix");
    m_uniforms.lightCount = m_shader.getLocation("lightCount");
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

void ShadingProgram::setFrame(const Camera &camera, const RenderData &data) {
    setCamera(camera);
    setGlobalData(data.globalData);
    setLights(data.lights);
    m_boundTextures.fill(nullptr);
}

void ShadingProgram::setShape(const RenderShapeData &shape) {
    setTransform(shape);
    setMaterial(shape);
    setTextureMap(shape);
    setEmissiveMap(shape);
}

void ShadingProgram::setCamera(const Camera &camera) {
    m_shader.setMat4(m_uniforms.viewMatrix, camera.getViewMatrix());
    m_shader.setMat4(m_uniforms.projMatrix, camera.getProjectionMatrix());
    m_shader.setVec3(m_uniforms.cameraPos, camera.getPose().position);
}

void ShadingProgram::setLights(const std::vector<SceneLightData> &lights) {
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

void ShadingProgram::setTransform(const RenderShapeData &shape) {
    m_shader.setMat4(m_uniforms.modelMatrix, shape.ctm);
    m_shader.setMat3(m_uniforms.normalMatrix, shape.normalMatrix);
}

void ShadingProgram::setTextureMap(const RenderShapeData &shape) {
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

void ShadingProgram::setEmissiveMap(const RenderShapeData &shape) {
    if (shape.emissiveMap == nullptr || !shape.emissiveEnabled) {
        bindTexture(m_placeholderTexture, EmissiveMapUnit);
        return;
    }
    bindTexture(*shape.emissiveMap, EmissiveMapUnit);
}

void ShadingProgram::bindTexture(const Texture2D &texture, int unit) {
    if (m_boundTextures[unit] == &texture) {
        return;
    }
    texture.bind(unit);
    m_boundTextures[unit] = &texture;
}
