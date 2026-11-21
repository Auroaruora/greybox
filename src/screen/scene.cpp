#include "scene.h"

#include "debug/debug.h"
#include "gl/meshcache.h"
#include "gl/texturecache.h"
#include "utils/paths.h"

#include <algorithm>
#include <filesystem>
#include <sstream>

static std::string getSidecarFile(const std::string &sceneFile, const std::string &suffix) {
    return std::filesystem::path(sceneFile).replace_extension().string() + suffix;
}

bool Scene::load(const std::string &filepath) {
    RenderData parsed;
    if (!SceneParser::parse(filepath, parsed)) {
        LOG_ERROR("failed to load " << filepath);
        return false;
    }

    m_data = std::move(parsed);
    m_filepath = filepath;
    loadLighting();
    loadViewpoints();
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

void Scene::loadLighting() {
    m_lightingGroups.clear();
    m_lightingName.clear();

    std::string filename = getSidecarFile(m_filepath, "_lighting.json");
    if (!std::filesystem::exists(Paths::getAssetPath(filename))) {
        return;
    }

    LightingReader reader(filename);
    if (reader.readJSON() && !reader.getGroups().empty()) {
        m_lightingGroups = reader.getGroups();
        setLighting(m_lightingGroups[0].name);
    }
}

void Scene::loadViewpoints() {
    m_viewpoints.clear();

    std::string filename = getSidecarFile(m_filepath, "_viewpoints.json");
    if (!std::filesystem::exists(Paths::getAssetPath(filename))) {
        return;
    }

    ViewpointReader reader(filename);
    if (reader.readJSON()) {
        m_viewpoints = reader.getViewpoints();
    }
}

void Scene::setLighting(const std::string &name) {
    auto found = std::find_if(m_lightingGroups.begin(), m_lightingGroups.end(), [&name](const LightingGroup &group) {
        return group.name == name;
    });
    if (found == m_lightingGroups.end()) {
        return;
    }
    const LightingGroup &group = *found;
    m_lightingName = group.name;
    m_data.lights = group.lights;
    m_data.globalData.ambientLight = group.ambient;

    for (RenderShapeData &shape : m_data.shapes) {
        shape.emissiveEnabled = true;
        SceneMaterial &material = shape.primitive.material;

        auto emissive = group.emissive.find(shape.primitive.name);
        if (emissive != group.emissive.end()) {
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

const std::vector<LightingGroup> &Scene::getLightingGroups() const {
    return m_lightingGroups;
}

const std::string &Scene::getLightingName() const {
    return m_lightingName;
}

const std::vector<Viewpoint> &Scene::getViewpoints() const {
    return m_viewpoints;
}
