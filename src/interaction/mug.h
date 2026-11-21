#pragma once

#include "interaction/interactable.h"

#include <glm/glm.hpp>

enum class MugState {
    Idle,
    Raising,
    Tilting,
    Drinking,
    Leveling,
    Lowering
};

class Mug : public Interactable {
public:
    explicit Mug(const std::string &name);

    void load(Scene &scene, MeshCache &meshCache) override;
    void interact(InteractionContext &context) override;
    bool update(InteractionContext &context, float deltaTime) override;
    const std::string &getIcon() const override;

private:
    static constexpr int FullSips = 4;

    void updateCoffee(Scene &scene);
    float getDuration() const;

    int m_coffee = -1;
    glm::mat4 m_coffeeFull = glm::mat4(1.0f);
    int m_sips = FullSips;

    MugState m_state = MugState::Idle;
    float m_timer = 0.0f;
    bool m_isLastSip = false;
    glm::vec3 m_mugRest = glm::vec3(0.0f);
    glm::vec3 m_mugHold = glm::vec3(0.0f);
    glm::vec3 m_tiltAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    float m_drop = 0.0f;
    float m_dropFrom = 0.0f;
    float m_dropTo = 0.0f;
};
