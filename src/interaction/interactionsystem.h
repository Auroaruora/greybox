#pragma once

#include "event/gameevent.h"
#include "interaction/interactable.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class InteractionSystem {
public:
    void add(std::unique_ptr<Interactable> item);
    void load(Scene &scene, MeshCache &meshCache, TextureCache &textureCache);
    void update(Scene &scene, const Camera &camera, const std::optional<Ray> &pointer, float deltaTime);

    const std::string &getIcon() const;
    const std::vector<GameEvent> &getEvents() const;

private:
    void updateActive(Scene &scene, const Camera &camera, float deltaTime);
    void pick(Scene &scene, const Camera &camera, const Ray &ray);

    std::vector<std::unique_ptr<Interactable>> m_items;
    std::vector<Interactable *> m_active;
    int m_hovered = -1;
    std::vector<GameEvent> m_events;
};
