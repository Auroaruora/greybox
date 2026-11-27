#include "scene.h"

#include "debug/debug.h"
#include "gl/meshcache.h"
#include "gl/texturecache.h"

#include <algorithm>
#include <sstream>

bool Scene::load(const std::string &filepath, MeshCache &meshCache) {
    auto getMaterial = [&meshCache](const std::string &meshfile) { return meshCache.getMaterialName(meshfile); };
    RenderData parsed;
    std::vector<LightingState> lightingStates;
    std::vector<Viewpoint> viewpoints;
    if (!SceneParser::parse(filepath, getMaterial, parsed, lightingStates, viewpoints)) {
        LOG_ERROR("failed to load " << filepath);
        return false;
    }

    m_data = std::move(parsed);
    m_filepath = filepath;
    m_lightingStates = std::move(lightingStates);
    m_viewpoints = std::move(viewpoints);
    m_lightingName.clear();
    if (!m_lightingStates.empty()) {
        setLighting(m_lightingStates[0].name);
    }
    LOG_DEBUG(getSummary());
    return true;
}

void Scene::loadMeshes(MeshCache &meshCache) {
    for (RenderShapeData &shape : m_data.shapes) {
        if (shape.primitive.type == PrimitiveType::PRIMITIVE_MESH) {
            shape.mesh = &meshCache.getMesh(shape.primitive.meshfile);
        }
    }
}

void Scene::loadTextures(TextureCache &textureCache) {
    for (RenderShapeData &shape : m_data.shapes) {
        const SceneFileMap &map = shape.primitive.material.textureMap;
        shape.textureMap = map.isUsed ? textureCache.getTexture(map.filename) : nullptr;
    }
}

std::string Scene::getSummary() const {
    int directional = 0, point = 0, spot = 0;
    for (const SceneLightData &light : m_data.lights) {
        switch (light.type) {
        case LightType::LIGHT_DIRECTIONAL: directional++; break;
        case LightType::LIGHT_POINT:       point++;       break;
        case LightType::LIGHT_SPOT:        spot++;        break;
        }
    }

    const glm::vec4 &pos  = m_data.cameraData.pos;
    const glm::vec4 &look = m_data.cameraData.look;

    std::ostringstream out;
    out << "loaded " << m_filepath << "\n"
        << "  shapes: " << m_data.shapes.size() << "\n"
        << "  lights: " << m_data.lights.size()
        << " (directional " << directional
        << ", point " << point
        << ", spot " << spot << ")\n"
        << "  camera: pos (" << pos.x << ", " << pos.y << ", " << pos.z << ")"
        << "  look (" << look.x << ", " << look.y << ", " << look.z << ")";
    return out.str();
}

void Scene::setLightEnabled(int index, bool enabled) {
    SceneLightData &light = m_data.lights[index];
    light.enabled = enabled;
    if (light.emitter.empty()) {
        return;
    }

    for (RenderShapeData &shape : m_data.shapes) {
        if (shape.primitive.name == light.emitter) {
            shape.emissiveEnabled = enabled;
        }
    }
}

int Scene::getLightIndex(const std::string &name) const {
    for (int i = 0; i < static_cast<int>(m_data.lights.size()); i++) {
        if (m_data.lights[i].name == name) {
            return i;
        }
    }
    return -1;
}

void Scene::setShapeTransform(int index, const glm::mat4 &ctm) {
    RenderShapeData &shape = m_data.shapes[index];
    shape.ctm = ctm;
    shape.inverseCtm = glm::inverse(ctm);
    shape.normalMatrix = glm::transpose(glm::mat3(shape.inverseCtm));
}

void Scene::setShapeVisible(int index, bool visible) {
    m_data.shapes[index].visible = visible;
}

void Scene::setShapeEmissiveMap(int index, const Texture2D *texture) {
    m_data.shapes[index].emissiveMap = texture;
}

void Scene::setLighting(const std::string &name) {
    auto found = std::find_if(m_lightingStates.begin(), m_lightingStates.end(), [&name](const LightingState &lighting) {
        return lighting.name == name;
    });
    if (found == m_lightingStates.end()) {
        return;
    }
    const LightingState &lighting = *found;
    m_lightingName = lighting.name;
    m_data.lights = lighting.lights;
    m_data.globalData.ambientLight = lighting.ambient;
    m_data.sky = lighting.sky;
    m_data.toonShade = lighting.toonShade;

    for (RenderShapeData &shape : m_data.shapes) {
        shape.emissiveEnabled = true;
        SceneMaterial &material = shape.primitive.material;

        auto emissive = lighting.emissive.find(shape.primitive.name);
        if (emissive != lighting.emissive.end()) {
            material.cEmissive = emissive->second;
        } else {
            material.cEmissive = SceneColor(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}

const std::string &Scene::getFilepath() const {
    return m_filepath;
}

const RenderData &Scene::getRenderData() const {
    return m_data;
}

const SceneCameraData &Scene::getCameraData() const {
    return m_data.cameraData;
}

const std::vector<SceneLightData> &Scene::getLights() const {
    return m_data.lights;
}

const std::vector<RenderShapeData> &Scene::getShapes() const {
    return m_data.shapes;
}

const std::vector<LightingState> &Scene::getLightingStates() const {
    return m_lightingStates;
}

const std::string &Scene::getLightingName() const {
    return m_lightingName;
}

const std::vector<Viewpoint> &Scene::getViewpoints() const {
    return m_viewpoints;
}
