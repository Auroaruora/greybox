#include "sceneparser.h"

#include "debug/debug.h"
#include "utils/assetreader.h"
#include "utils/paths.h"
#include "utils/scenereader.h"

#include <glm/gtx/transform.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <set>
#include <stdexcept>

namespace {

const float DefaultFar = 100.0f;
const double DefaultShininess = 8.0;
const std::string MaybeGroup = "maybe";
const glm::dvec3 DefaultDiffuse(0.5);
const glm::dvec3 DefaultSpecular(0.1);
const glm::dvec3 Forward(0.0, 0.0, -1.0);
const glm::vec3 Attenuation(0.0f, 0.0f, 1.0f);
const std::map<std::string, LightType> LightTypes = {
    {"sun", LightType::LIGHT_DIRECTIONAL}, {"point", LightType::LIGHT_POINT}, {"spot", LightType::LIGHT_SPOT}};

struct Input {
    SceneSource source;
    Palette palette;
    std::map<std::string, const LayoutObject *> objects;
    std::map<std::string, AssetDoc> docs;
};

struct Part {
    std::string key;
    glm::dmat4 matrix;
    ScenePrimitive primitive;
    std::optional<glm::dvec3> emission;
};

struct Light {
    std::string name;
    LightType type;
    glm::dvec3 color;
    glm::dvec3 position;
    glm::dvec3 direction;
    double angle = 0.0;
    double penumbra = 0.0;
    std::string emitter;
};

struct Glow {
    std::map<std::string, GlowSetting> current;
    std::set<std::string> fromStates;
};

struct SceneTree {
    std::vector<std::unique_ptr<SceneNode>> nodes;
    std::vector<std::unique_ptr<SceneTransformation>> transformations;
    std::vector<std::unique_ptr<ScenePrimitive>> primitives;
    std::vector<std::unique_ptr<SceneLight>> lights;

    SceneNode *addNode(SceneNode *parent) {
        nodes.push_back(std::make_unique<SceneNode>());
        if (parent != nullptr) {
            parent->children.push_back(nodes.back().get());
        }
        return nodes.back().get();
    }

    void addMatrix(SceneNode *node, const glm::mat4 &matrix) {
        transformations.push_back(std::make_unique<SceneTransformation>());
        transformations.back()->type = TransformationType::TRANSFORMATION_MATRIX;
        transformations.back()->matrix = matrix;
        node->transformations.push_back(transformations.back().get());
    }
};

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

bool isBefore(const std::string &a, const std::string &b) {
    std::string la = lower(a), lb = lower(b);
    return la != lb ? la < lb : a < b;
}

glm::vec3 toFloat(const glm::dvec3 &value) {
    return glm::vec3(static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z));
}

glm::mat4 toFloat(const glm::dmat4 &value) {
    glm::mat4 matrix(1.0f);
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            matrix[column][row] = static_cast<float>(value[column][row]);
        }
    }
    return matrix;
}

float toRadians(double degrees) {
    return glm::radians(static_cast<float>(degrees));
}

glm::dmat4 placement(const LayoutObject &object) {
    glm::dmat4 matrix = glm::translate(glm::dmat4(1.0), object.position);
    matrix = glm::rotate(matrix, glm::radians(object.rotation.y), glm::dvec3(0.0, 1.0, 0.0));
    matrix = glm::rotate(matrix, glm::radians(object.rotation.x), glm::dvec3(1.0, 0.0, 0.0));
    return glm::rotate(matrix, glm::radians(object.rotation.z), glm::dvec3(0.0, 0.0, 1.0));
}

CameraPose camera(const ViewSource &view) {
    CameraPose pose;
    pose.position = toFloat(view.position);
    pose.look = toFloat(view.look);
    pose.up = toFloat(view.up);
    pose.heightAngle = toRadians(view.heightAngle);
    return pose;
}

const LookChoice &getChoice(const Input &input, const std::string &name) {
    static const LookChoice None;
    auto found = input.source.look.find(name);
    return found != input.source.look.end() ? found->second : None;
}

const AssetDoc &getDoc(Input &input, const std::string &asset) {
    auto found = input.docs.find(asset);
    if (found == input.docs.end()) {
        AssetDoc doc;
        if (!AssetReader::load(asset, doc)) {
            throw std::runtime_error("cannot read asset " + asset);
        }
        found = input.docs.emplace(asset, doc).first;
    }
    return found->second;
}

std::string getColour(const AssetDoc &doc, const LookChoice &choice, const std::string &shape) {
    if (choice.colour) {
        return *choice.colour;
    }
    for (const auto &[name, set] : doc.colours) {
        if (std::find(set.shapes.begin(), set.shapes.end(), shape) != set.shapes.end()) {
            return name;
        }
    }
    return "";
}

std::string getPartMaterial(const AssetDoc &doc, const std::string &colour, const std::string &part) {
    for (const auto &[name, set] : doc.colours) {
        if (name != colour) {
            continue;
        }
        auto found = set.parts.find(part);
        if (found == set.parts.end()) {
            found = set.parts.find("*");
        }
        return found == set.parts.end() || found->second.empty() ? "" : found->second.front();
    }
    return "";
}

glm::dmat4 holderMatrix(const Input &input, const std::string &name) {
    glm::dmat4 matrix = placement(*input.objects.at(name));
    const LookChoice &choice = getChoice(input, name);
    return choice.scale ? glm::scale(matrix, *choice.scale) : matrix;
}

void setMaterial(const Input &input, const AssetDoc &doc, const std::string &name, Part &part) {
    SceneMaterial &material = part.primitive.material;
    auto texture = doc.textures.find(name);
    if (texture != doc.textures.end()) {
        material.cDiffuse = SceneColor(1.0f, 1.0f, 1.0f, 1.0f);
        material.textureMap.isUsed = true;
        material.textureMap.filename = texture->second;
        material.textureMap.repeatU = 1.0f;
        material.textureMap.repeatV = 1.0f;
        material.blend = 1.0f;
        return;
    }
    auto colour = input.palette.find(name);
    if (colour != input.palette.end()) {
        material.cDiffuse = SceneColor(toFloat(colour->second.linear), 1.0f);
        part.emission = colour->second.emission;
        return;
    }
    material.cDiffuse = SceneColor(toFloat(DefaultDiffuse), 1.0f);
}

std::vector<Part> collectParts(Input &input, const SceneParser::MaterialLookup &getMeshMaterial) {
    const SceneSettings &settings = input.source.settings;
    glm::vec3 specular = toFloat(settings.specular.value_or(DefaultSpecular));
    float shininess = static_cast<float>(settings.shininess.value_or(DefaultShininess));

    std::vector<const LayoutObject *> objects;
    for (const LayoutObject &object : input.source.layout) {
        objects.push_back(&object);
    }
    std::sort(objects.begin(), objects.end(), [](const LayoutObject *a, const LayoutObject *b) { return isBefore(a->name, b->name); });

    std::vector<Part> parts;
    for (const LayoutObject *object : objects) {
        if (object->group == MaybeGroup) {
            continue;
        }
        const std::string &name = object->name;
        const AssetDoc &doc = getDoc(input, object->asset);
        const LookChoice &choice = getChoice(input, name);
        std::string shape = choice.shape.value_or(doc.shapes.at(0));
        std::string colour = getColour(doc, choice, shape);
        glm::dmat4 matrix = holderMatrix(input, name);

        std::vector<std::string> names = doc.parts;
        std::sort(names.begin(), names.end(), [&name](const std::string &a, const std::string &b) {
            return isBefore(name + "_" + a, name + "_" + b);
        });
        for (const std::string &partName : names) {
            std::string meshfile = "models/" + object->asset + "/" + shape + "_" + partName + ".obj";
            if (!std::filesystem::exists(Paths::getAssetPath(meshfile))) {
                continue;
            }
            std::string chosen = getPartMaterial(doc, colour, partName);
            if (chosen.empty()) {
                chosen = getMeshMaterial(meshfile);
            }
            Part part;
            part.key = name + "_" + partName;
            part.matrix = matrix;
            part.primitive.type = PrimitiveType::PRIMITIVE_MESH;
            part.primitive.name = part.key;
            part.primitive.meshfile = meshfile;
            part.primitive.material.clear();
            part.primitive.material.cSpecular = SceneColor(specular, 1.0f);
            part.primitive.material.shininess = shininess;
            setMaterial(input, doc, chosen, part);
            parts.push_back(part);
        }
    }
    return parts;
}

void applyGlow(const Input &input, const LightingState &state, Glow &glow) {
    for (const LightSource &light : input.source.lights) {
        auto setting = state.lights.find(light.name);
        if (light.attach.empty() || light.glowPart.empty() || setting == state.lights.end() || !setting->second.glow) {
            continue;
        }
        glow.current[light.attach + "_" + light.glowPart] = *setting->second.glow;
    }
    std::set<std::string> fromState;
    for (const auto &[key, value] : state.glow) {
        glow.current[key] = value;
        fromState.insert(key);
    }
    for (const std::string &key : glow.fromStates) {
        if (!fromState.contains(key)) {
            glow.current[key] = GlowSetting();
        }
    }
    glow.fromStates = fromState;
}

bool getEmission(const Glow &glow, const Part &part, SceneColor &colour) {
    auto found = glow.current.find(part.key);
    if (found == glow.current.end()) {
        if (!part.emission) {
            return false;
        }
        colour = SceneColor(toFloat(*part.emission), 1.0f);
        return true;
    }
    if (found->second.strength <= 0.0) {
        return false;
    }
    colour = SceneColor(toFloat(found->second.color * found->second.strength), 1.0f);
    return true;
}

bool isAttached(const Input &input, const LightSource &light) {
    return !light.attach.empty() && input.objects.contains(light.attach);
}

bool isHidden(const Input &input, const LightSource &light) {
    return isAttached(input, light) && input.objects.at(light.attach)->group == MaybeGroup;
}

Light makeLight(const Input &input, const LightSource &source, const LightSetting &setting) {
    Light light;
    light.name = source.name;
    light.type = LightTypes.at(source.type);
    light.color = setting.color * setting.intensity;
    light.position = source.position.value_or(glm::dvec3(0.0));
    light.direction = setting.direction.value_or(source.direction.value_or(Forward));
    if (isAttached(input, source)) {
        glm::dmat4 holder = holderMatrix(input, source.attach);
        light.position = glm::dvec3(holder * glm::dvec4(light.position, 1.0));
        light.direction = glm::normalize(glm::dmat3(holder) * light.direction);
    }
    if (light.type == LightType::LIGHT_SPOT) {
        light.angle = source.angle;
        light.penumbra = source.penumbra;
    }
    if (!source.attach.empty() && !source.glowPart.empty()) {
        light.emitter = source.attach + "_" + source.glowPart;
    }
    return light;
}

std::vector<Light> makeLights(const Input &input, const LightingState &state) {
    std::vector<const LightSource *> sources;
    for (const LightSource &source : input.source.lights) {
        sources.push_back(&source);
    }
    std::sort(sources.begin(), sources.end(), [](const LightSource *a, const LightSource *b) { return isBefore(a->name, b->name); });

    std::vector<Light> result;
    for (const LightSource *source : sources) {
        if (source->type == "glow" || isHidden(input, *source)) {
            continue;
        }
        auto setting = state.lights.find(source->name);
        if (setting == state.lights.end() || setting->second.intensity <= 0.0) {
            continue;
        }
        result.push_back(makeLight(input, *source, setting->second));
    }
    return result;
}

SceneLightData toLightData(const Light &light, int id) {
    SceneLightData data;
    data.id = id;
    data.type = light.type;
    data.name = light.name;
    data.emitter = light.emitter;
    data.color = SceneColor(toFloat(light.color), 1.0f);
    data.function = Attenuation;
    data.pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    data.dir = glm::vec4(0.0f);
    data.penumbra = light.type == LightType::LIGHT_SPOT ? toRadians(light.penumbra) : 0.0f;
    data.angle = light.type == LightType::LIGHT_SPOT ? toRadians(light.angle) : 0.0f;
    data.width = 0.0f;
    data.height = 0.0f;
    if (light.type != LightType::LIGHT_POINT) {
        data.dir = glm::vec4(toFloat(light.direction), 0.0f);
    }
    if (light.type != LightType::LIGHT_DIRECTIONAL) {
        data.pos = glm::vec4(toFloat(light.position), 1.0f);
    }
    return data;
}

void addLight(SceneTree &tree, SceneNode *root, const Light &light, int id) {
    SceneNode *node = tree.addNode(root);
    tree.lights.push_back(std::make_unique<SceneLight>());
    SceneLight *sceneLight = tree.lights.back().get();
    node->lights.push_back(sceneLight);
    SceneLightData data = toLightData(light, id);
    sceneLight->id = id;
    sceneLight->type = data.type;
    sceneLight->name = data.name;
    sceneLight->emitter = data.emitter;
    sceneLight->color = data.color;
    sceneLight->function = data.function;
    sceneLight->penumbra = data.penumbra;
    sceneLight->angle = data.angle;
    sceneLight->dir = data.dir;
    if (light.type != LightType::LIGHT_DIRECTIONAL) {
        glm::mat4 translation(1.0f);
        translation[3] = data.pos;
        tree.addMatrix(node, translation);
    }
}

void addPart(SceneTree &tree, SceneNode *root, const Part &part, const Glow &glow) {
    SceneNode *node = tree.addNode(root);
    tree.addMatrix(node, toFloat(part.matrix));
    tree.primitives.push_back(std::make_unique<ScenePrimitive>(part.primitive));
    ScenePrimitive *primitive = tree.primitives.back().get();
    auto found = glow.current.find(part.key);
    if (found != glow.current.end() && found->second.base) {
        primitive->material.cDiffuse = SceneColor(toFloat(*found->second.base), 1.0f);
    }
    SceneColor emission(0.0f, 0.0f, 0.0f, 1.0f);
    getEmission(glow, part, emission);
    primitive->material.cEmissive = emission;
    node->primitives.push_back(primitive);
}

const LightingState &getState(const Input &input, const std::string &name) {
    for (const LightingState &state : input.source.states) {
        if (state.name == name) {
            return state;
        }
    }
    throw std::runtime_error("unknown state: " + name);
}

const ViewSource &getView(const Input &input, const std::string &name) {
    for (const ViewSource &view : input.source.viewpoints) {
        if (view.name == name) {
            return view;
        }
    }
    throw std::runtime_error("unknown viewpoint: " + name);
}

void traverse(SceneNode *node, glm::mat4 ctm, RenderData &renderData);

void buildScene(const Input &input, const std::vector<Part> &parts, RenderData &renderData) {
    const SceneSettings &settings = input.source.settings;
    const LightingState &state = getState(input, settings.startState);
    Glow glow;
    applyGlow(input, state, glow);

    SceneTree tree;
    SceneNode *root = tree.addNode(nullptr);
    std::vector<Light> lights = makeLights(input, state);
    for (int i = 0; i < static_cast<int>(lights.size()); i++) {
        addLight(tree, root, lights[i], i);
    }
    for (const Part &part : parts) {
        addPart(tree, root, part, glow);
    }

    renderData.globalData = {};
    renderData.globalData.ka = 1.0f;
    renderData.globalData.kd = 1.0f;
    renderData.globalData.ks = 1.0f;
    renderData.globalData.kt = 0.0f;
    renderData.globalData.ambientLight = toFloat(state.world);
    renderData.globalData.shading = settings.shading;

    CameraPose pose = camera(getView(input, settings.startViewpoint));
    renderData.cameraData = {};
    renderData.cameraData.pos = glm::vec4(pose.position, 1.0f);
    renderData.cameraData.look = glm::vec4(pose.look, 0.0f);
    renderData.cameraData.up = glm::vec4(pose.up, 0.0f);
    renderData.cameraData.heightAngle = pose.heightAngle;
    renderData.cameraData.far = settings.far ? static_cast<float>(*settings.far) : DefaultFar;

    renderData.shapes.clear();
    renderData.lights.clear();
    traverse(root, glm::mat4(1.0f), renderData);
}

std::vector<LightingGroup> buildLighting(const Input &input, const std::vector<Part> &parts) {
    const std::string &startState = input.source.settings.startState;
    Glow glow;
    applyGlow(input, getState(input, startState), glow);
    std::vector<LightingGroup> result(1);
    for (const LightingState &state : input.source.states) {
        applyGlow(input, state, glow);
        LightingGroup group;
        group.name = state.name;
        group.ambient = toFloat(state.world);
        std::vector<Light> lights = makeLights(input, state);
        for (int i = 0; i < static_cast<int>(lights.size()); i++) {
            group.lights.push_back(toLightData(lights[i], i));
        }
        for (const Part &part : parts) {
            SceneColor colour;
            if (getEmission(glow, part, colour)) {
                group.emissive[part.key] = colour;
            }
        }
        if (state.name == startState) {
            result[0] = group;
        } else {
            result.push_back(group);
        }
    }
    return result;
}

std::vector<Viewpoint> buildViewpoints(const Input &input) {
    std::vector<Viewpoint> result;
    for (const ViewSource &view : input.source.viewpoints) {
        result.push_back({view.name, camera(view)});
    }
    std::sort(result.begin(), result.end(), [](const Viewpoint &a, const Viewpoint &b) { return a.name < b.name; });
    return result;
}

glm::mat4 getTransformMatrix(const SceneTransformation* t) {
    switch (t->type) {
    case TransformationType::TRANSFORMATION_TRANSLATE:
        return glm::translate(t->translate);
    case TransformationType::TRANSFORMATION_SCALE:
        return glm::scale(t->scale);
    case TransformationType::TRANSFORMATION_ROTATE:
        return glm::rotate(t->angle, t->rotate);
    case TransformationType::TRANSFORMATION_MATRIX:
        return t->matrix;
    }
    return glm::mat4(1.0f);
}

void traverse(SceneNode* node, glm::mat4 ctm, RenderData& renderData) {
    for (SceneTransformation* t : node->transformations) {
        ctm = ctm * getTransformMatrix(t);
    }

    for (ScenePrimitive* prim : node->primitives) {
        RenderShapeData shape;
        shape.primitive  = *prim;
        shape.ctm        = ctm;
        shape.inverseCtm = glm::inverse(ctm);
        shape.normalMatrix = glm::transpose(glm::mat3(shape.inverseCtm));
        renderData.shapes.push_back(shape);
    }

    for (SceneLight* light : node->lights) {
        SceneLightData lightData;
        lightData.id       = light->id;
        lightData.type     = light->type;
        lightData.name     = light->name;
        lightData.emitter  = light->emitter;
        lightData.color    = light->color;
        lightData.function = light->function;
        lightData.penumbra = light->penumbra;
        lightData.angle    = light->angle;
        lightData.width    = light->width;
        lightData.height   = light->height;
        lightData.pos = ctm * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);        // w = 1: a point, so translation applies
        lightData.dir = ctm * glm::vec4(glm::vec3(light->dir), 0.0f);   // w = 0: a direction, so translation is ignored
        renderData.lights.push_back(lightData);
    }

    for (SceneNode* child : node->children) {
        traverse(child, ctm, renderData);
    }
}

}

bool SceneParser::parse(const std::string &lookFile, const MaterialLookup &getMeshMaterial, RenderData &renderData,
                        std::vector<LightingGroup> &lightingGroups, std::vector<Viewpoint> &viewpoints) {
    Input input;
    if (!SceneReader::load(lookFile, input.source) || !AssetReader::loadPalette(input.palette)) {
        return false;
    }
    for (const LayoutObject &object : input.source.layout) {
        input.objects[object.name] = &object;
    }
    try {
        std::vector<Part> parts = collectParts(input, getMeshMaterial);
        buildScene(input, parts, renderData);
        lightingGroups = buildLighting(input, parts);
        viewpoints = buildViewpoints(input);
    } catch (const std::exception &error) {
        LOG_ERROR(lookFile << ": " << error.what());
        return false;
    }
    return true;
}
