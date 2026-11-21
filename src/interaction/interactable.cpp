#include "interactable.h"

#include "gl/meshcache.h"
#include "screen/scene.h"

Interactable::Interactable(const std::string &name, const std::string &icon) {
    m_name = name;
    m_icon = icon;
}

void Interactable::load(Scene &scene, MeshCache &meshCache) {
    m_shapes.clear();
    m_aabbs.clear();
    m_bases.clear();
    m_transform = glm::mat4(1.0f);

    std::string partPrefix = m_name + "_";
    const std::vector<RenderShapeData> &shapes = scene.getShapes();
    for (int i = 0; i < static_cast<int>(shapes.size()); i++) {
        const ScenePrimitive &primitive = shapes[i].primitive;
        if (primitive.type != PrimitiveType::PRIMITIVE_MESH) {
            continue;
        }
        if (primitive.name != m_name && !primitive.name.starts_with(partPrefix)) {
            continue;
        }

        const AABB &aabb = meshCache.getAABB(primitive.meshfile);
        if (!aabb.isValid()) {
            continue;
        }
        m_shapes.push_back(i);
        m_aabbs.push_back(aabb);
        m_bases.push_back(shapes[i].ctm);
    }
    updateWorldAABB(scene);
}

void Interactable::updateWorldAABB(const Scene &scene) {
    const std::vector<RenderShapeData> &shapes = scene.getShapes();
    m_worldAABB = AABB();
    for (size_t i = 0; i < m_shapes.size(); i++) {
        m_worldAABB.expand(m_aabbs[i], shapes[m_shapes[i]].ctm);
    }
}

int Interactable::getPart(const Scene &scene, const std::string &name) const {
    const std::vector<RenderShapeData> &shapes = scene.getShapes();
    for (int i = 0; i < static_cast<int>(m_shapes.size()); i++) {
        if (shapes[m_shapes[i]].primitive.name == name) {
            return i;
        }
    }
    return -1;
}

const glm::mat4 &Interactable::getPartBase(int part) const {
    return m_bases[part];
}

void Interactable::setPartBase(Scene &scene, int part, const glm::mat4 &ctm) {
    m_bases[part] = ctm;
    scene.setShapeTransform(m_shapes[part], m_transform * ctm);
    updateWorldAABB(scene);
}

void Interactable::setPartVisible(Scene &scene, int part, bool visible) const {
    scene.setShapeVisible(m_shapes[part], visible);
}

void Interactable::setPartEmissiveMap(Scene &scene, int part, const Texture2D *texture) const {
    scene.setShapeEmissiveMap(m_shapes[part], texture);
}

void Interactable::setTransform(Scene &scene, const glm::mat4 &transform) {
    m_transform = transform;
    for (size_t i = 0; i < m_shapes.size(); i++) {
        scene.setShapeTransform(m_shapes[i], transform * m_bases[i]);
    }
    updateWorldAABB(scene);
}

void Interactable::interact(InteractionContext &context) {
}

bool Interactable::update(InteractionContext &context, float deltaTime) {
    return false;
}

bool Interactable::intersect(const Scene &scene, const Ray &ray, float &distance) const {
    float ignored = 0.0f;
    if (!m_worldAABB.intersect(ray, ignored)) {
        return false;
    }

    const std::vector<RenderShapeData> &shapes = scene.getShapes();
    bool hit = false;

    for (size_t i = 0; i < m_shapes.size(); i++) {
        const glm::mat4 &inverse = shapes[m_shapes[i]].inverseCtm;
        Ray local;
        local.origin = glm::vec3(inverse * glm::vec4(ray.origin, 1.0f));
        local.direction = glm::vec3(inverse * glm::vec4(ray.direction, 0.0f));

        float current = 0.0f;
        if (m_aabbs[i].intersect(local, current) && (!hit || current < distance)) {
            distance = current;
            hit = true;
        }
    }
    return hit;
}

glm::vec3 Interactable::getCenter() const {
    return m_worldAABB.getCenter();
}

const std::string &Interactable::getName() const {
    return m_name;
}

const std::string &Interactable::getIcon() const {
    return m_icon;
}

const std::vector<int> &Interactable::getShapes() const {
    return m_shapes;
}
