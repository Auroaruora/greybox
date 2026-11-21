#include "scenefilereader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

static glm::vec3 readVec3(const json &value) {
    return glm::vec3(value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>());
}

static SceneColor readColor(const json &object, const char *key) {
    if (!object.contains(key)) {
        return SceneColor(0.0f, 0.0f, 0.0f, 1.0f);
    }
    return SceneColor(readVec3(object.at(key)), 1.0f);
}

static glm::mat4 readMatrix(const json &rows) {
    glm::mat4 matrix(1.0f);
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            matrix[column][row] = rows.at(row).at(column).get<float>();
        }
    }
    return matrix;
}

static void addMatrix(SceneNode *node, const glm::mat4 &matrix) {
    SceneTransformation *transformation = new SceneTransformation();
    transformation->type = TransformationType::TRANSFORMATION_MATRIX;
    transformation->matrix = matrix;
    node->transformations.push_back(transformation);
}

static void readObject(const json &object, const json &models, SceneNode *node) {
    addMatrix(node, object.contains("matrix") ? readMatrix(object.at("matrix")) : glm::mat4(1.0f));

    const json &model = models.at(object.at("model").get<std::string>());

    ScenePrimitive *primitive = new ScenePrimitive();
    node->primitives.push_back(primitive);

    primitive->type = PrimitiveType::PRIMITIVE_MESH;
    primitive->name = object.at("model").get<std::string>();
    primitive->meshfile = model.at("mesh").get<std::string>();
    primitive->material.clear();
    primitive->material.cDiffuse = readColor(model, "diffuse");
    primitive->material.cSpecular = readColor(model, "specular");
    primitive->material.cEmissive = readColor(model, "emissive");
    primitive->material.shininess = model.value("shininess", 1.0f);
    if (model.contains("textureFile")) {
        SceneFileMap &map = primitive->material.textureMap;
        map.isUsed = true;
        map.filename = model.at("textureFile").get<std::string>();
        map.repeatU = model.value("textureU", 1.0f);
        map.repeatV = model.value("textureV", 1.0f);
        primitive->material.blend = model.value("blend", 1.0f);
    }
}

static void readLight(const json &data, int id, SceneNode *node) {
    SceneLight *light = new SceneLight();
    node->lights.push_back(light);

    light->id = id;
    light->name = data.value("name", std::string());
    light->emitter = data.value("emitter", std::string());
    light->color = SceneColor(readVec3(data.at("color")), 1.0f);
    light->function = data.contains("attenuationCoeff") ? readVec3(data.at("attenuationCoeff")) : glm::vec3(1.0f, 0.0f, 0.0f);

    std::string type = data.at("type").get<std::string>();
    if (type == "directional") {
        light->type = LightType::LIGHT_DIRECTIONAL;
    } else if (type == "point") {
        light->type = LightType::LIGHT_POINT;
    } else if (type == "spot") {
        light->type = LightType::LIGHT_SPOT;
        light->penumbra = glm::radians(data.at("penumbra").get<float>());
        light->angle = glm::radians(data.at("angle").get<float>());
    } else {
        throw std::runtime_error("unknown light type: " + type);
    }

    if (light->type != LightType::LIGHT_POINT) {
        light->dir = glm::vec4(readVec3(data.at("direction")), 0.0f);
    }
    if (light->type != LightType::LIGHT_DIRECTIONAL) {
        glm::mat4 translation(1.0f);
        translation[3] = glm::vec4(readVec3(data.at("position")), 1.0f);
        addMatrix(node, translation);
    }
}

ScenefileReader::ScenefileReader(const std::string &filename) {
    m_filename = filename;
}

ScenefileReader::~ScenefileReader() {
    for (SceneNode *node : m_nodes) {
        for (SceneTransformation *transformation : node->transformations) {
            delete transformation;
        }
        for (ScenePrimitive *primitive : node->primitives) {
            delete primitive;
        }
        for (SceneLight *light : node->lights) {
            delete light;
        }
        delete node;
    }
}

SceneNode *ScenefileReader::addNode(SceneNode *parent) {
    SceneNode *node = new SceneNode();
    m_nodes.push_back(node);
    if (parent != nullptr) {
        parent->children.push_back(node);
    }
    return node;
}

bool ScenefileReader::readJSON() {
    std::ifstream file(Paths::getAssetPath(m_filename));
    if (!file.is_open()) {
        LOG_ERROR("cannot open " << m_filename);
        return false;
    }

    try {
        json scene = json::parse(file);

        const json &global = scene.at("globalData");
        m_globalData.ka = global.at("ambientCoeff").get<float>();
        m_globalData.kd = global.at("diffuseCoeff").get<float>();
        m_globalData.ks = global.at("specularCoeff").get<float>();
        m_globalData.kt = 0.0f;
        m_globalData.ambientLight = global.contains("ambientLight") ? readVec3(global.at("ambientLight")) : glm::vec3(0.0f);

        const json &camera = scene.at("cameraData");
        glm::vec3 position = readVec3(camera.at("position"));
        glm::vec3 look = camera.contains("focus") ? readVec3(camera.at("focus")) - position : readVec3(camera.at("look"));
        m_cameraData.pos = glm::vec4(position, 1.0f);
        m_cameraData.look = glm::vec4(look, 0.0f);
        m_cameraData.up = glm::vec4(readVec3(camera.at("up")), 0.0f);
        m_cameraData.heightAngle = glm::radians(camera.at("heightAngle").get<float>());

        m_root = addNode(nullptr);

        int lightId = 0;
        if (scene.contains("lights")) {
            for (const json &light : scene.at("lights")) {
                readLight(light, lightId++, addNode(m_root));
            }
        }
        if (scene.contains("objects")) {
            const json &models = scene.at("models");
            for (const json &object : scene.at("objects")) {
                readObject(object, models, addNode(m_root));
            }
        }
    } catch (const std::exception &error) {
        LOG_ERROR(m_filename << ": " << error.what());
        return false;
    }
    return true;
}

SceneGlobalData ScenefileReader::getGlobalData() const {
    return m_globalData;
}

SceneCameraData ScenefileReader::getCameraData() const {
    return m_cameraData;
}

SceneNode *ScenefileReader::getRootNode() const {
    return m_root;
}