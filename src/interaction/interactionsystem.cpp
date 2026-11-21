#include "interactionsystem.h"

#include "debug/debug.h"
#include "input/input.h"
#include "screen/scene.h"

#include <algorithm>
#include <sstream>
#include <utility>

static const std::string DefaultIcon = "default";

void InteractionSystem::add(std::unique_ptr<Interactable> item) {
    m_items.push_back(std::move(item));
}

void InteractionSystem::load(Scene &scene, MeshCache &meshCache) {
    m_hovered = -1;

    std::ostringstream summary;
    for (std::unique_ptr<Interactable> &item : m_items) {
        item->load(scene, meshCache);
        summary << " " << item->getName() << "(" << item->getShapes().size() << ")";
    }
    LOG_INFO(scene.getFilepath() << " interactables:" << summary.str());
}

void InteractionSystem::update(Scene &scene, const Camera &camera, const std::optional<Ray> &pointer, float deltaTime) {
    m_events.clear();
    updateActive(scene, camera, deltaTime);
    if (pointer.has_value()) {
        pick(scene, camera, *pointer);
    } else {
        m_hovered = -1;
    }
}

void InteractionSystem::updateActive(Scene &scene, const Camera &camera, float deltaTime) {
    if (m_active.empty()) {
        return;
    }

    InteractionContext context{scene, camera};
    for (size_t i = 0; i < m_active.size();) {
        if (m_active[i]->update(context, deltaTime)) {
            i++;
        } else {
            m_active.erase(m_active.begin() + i);
        }
    }
}

void InteractionSystem::pick(Scene &scene, const Camera &camera, const Ray &ray) {
    m_hovered = -1;
    float nearest = 0.0f;
    for (int i = 0; i < static_cast<int>(m_items.size()); i++) {
        float distance = 0.0f;
        if (!m_items[i]->intersect(scene, ray, distance)) {
            continue;
        }
        if (m_hovered < 0 || distance < nearest) {
            m_hovered = i;
            nearest = distance;
        }
    }

    if (m_hovered >= 0 && Input::getButtonDown(Action::Interact)) {
        Interactable *item = m_items[m_hovered].get();
        InteractionContext context{scene, camera};
        item->interact(context);
        m_events.push_back(GameEvent{EventType::Click, item->getName()});
        if (std::find(m_active.begin(), m_active.end(), item) == m_active.end()) {
            m_active.push_back(item);
        }
    }
}

const std::string &InteractionSystem::getIcon() const {
    return m_hovered >= 0 ? m_items[m_hovered]->getIcon() : DefaultIcon;
}

const std::vector<GameEvent> &InteractionSystem::getEvents() const {
    return m_events;
}
