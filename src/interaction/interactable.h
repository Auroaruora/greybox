#pragma once

#include "interaction/aabb.h"
#include "interaction/ray.h"

#include <string>
#include <vector>

class Camera;
class MeshCache;
class TextureCache;
class Scene;
class Texture2D;

struct InteractionContext {
    Scene &scene;
    const Camera &camera;
};

class Interactable {
public:
    Interactable(const std::string &name, const std::string &icon);
    virtual ~Interactable() = default;

    virtual void load(Scene &scene, MeshCache &meshCache, TextureCache &textureCache);
    bool intersect(const Scene &scene, const Ray &ray, float &distance) const;

    virtual void interact(InteractionContext &context);
    virtual bool update(InteractionContext &context, float deltaTime);
    virtual const std::string &getIcon() const;

    const std::string &getName() const;
    const std::vector<int> &getShapes() const;

protected:
    int getPart(const Scene &scene, const std::string &name) const;
    const glm::mat4 &getPartBase(int part) const;
    glm::vec3 getCenter() const;
    void setPartBase(Scene &scene, int part, const glm::mat4 &ctm);
    void setPartVisible(Scene &scene, int part, bool visible) const;
    void setPartEmissiveMap(Scene &scene, int part, const Texture2D *texture) const;
    void setTransform(Scene &scene, const glm::mat4 &transform);

private:
    void updateWorldAABB(const Scene &scene);

    std::string m_name;
    std::string m_icon;
    std::vector<int> m_shapes;
    std::vector<AABB> m_aabbs;
    std::vector<glm::mat4> m_bases;
    glm::mat4 m_transform = glm::mat4(1.0f);
    AABB m_worldAABB;
};
