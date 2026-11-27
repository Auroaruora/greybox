#include "mug.h"

#include "camera/camera.h"
#include "debug/debug.h"

#include <glm/gtx/transform.hpp>

static const std::string CoffeeIcon = "coffee";
static const std::string RefillIcon = "refill";
static const std::string CoffeeSuffix = "_coffee";

static const float SipDrop = 0.02f;

static const float HoldForward = 0.30f;
static const float HoldRight = 0.0f;
static const float HoldDown = 0.10f;

static const float TiltAngle = glm::radians(50.0f);
static const float LastTiltAngle = glm::radians(80.0f);
static const float LastDrinkTime = 0.9f;

struct MugKeyframe {
    float duration;
    float lift;
    float tilt;
};

static const MugKeyframe Keyframes[] = {
    {.duration = 0.00f, .lift = 0.0f, .tilt = 0.0f},
    {.duration = 0.45f, .lift = 1.0f, .tilt = 0.0f},
    {.duration = 0.35f, .lift = 1.0f, .tilt = 1.0f},
    {.duration = 0.50f, .lift = 1.0f, .tilt = 1.0f},
    {.duration = 0.30f, .lift = 1.0f, .tilt = 0.0f},
    {.duration = 0.45f, .lift = 0.0f, .tilt = 0.0f},
};

static float ease(float t) {
    return t * t * (3.0f - 2.0f * t);
}

Mug::Mug(const std::string &name) : Interactable(name, CoffeeIcon) {
}

void Mug::load(Scene &scene, MeshCache &meshCache, TextureCache &textureCache) {
    Interactable::load(scene, meshCache, textureCache);

    m_mugRest = getCenter();

    m_coffee = getPart(scene, getName() + CoffeeSuffix);
    if (m_coffee >= 0) {
        m_coffeeFull = getPartBase(m_coffee);
        updateCoffee(scene);
    }
}

void Mug::interact(InteractionContext &context) {
    if (m_state != MugState::Idle) {
        return;
    }

    if (m_sips == 0) {
        m_sips = FullSips;
        m_drop = 0.0f;
        updateCoffee(context.scene);
        LOG_INFO(getName() << ": refilled");
        return;
    }

    m_isLastSip = m_sips == 1;
    m_sips--;
    m_dropFrom = m_drop;
    m_dropTo = static_cast<float>(FullSips - m_sips) * SipDrop;

    const CameraPose &pose = context.camera.getPose();
    glm::vec3 look = pose.look;
    glm::vec3 right = glm::normalize(glm::cross(look, pose.up));
    glm::vec3 up = glm::cross(right, look);
    m_mugHold = pose.position + look * HoldForward + right * HoldRight - up * HoldDown;
    m_tiltAxis = right;

    m_state = MugState::Raising;
    m_timer = 0.0f;
    LOG_INFO(getName() << ": " << m_sips << " sips left");
}

bool Mug::update(InteractionContext &context, float deltaTime) {
    if (m_state == MugState::Idle) {
        return false;
    }

    int stage = static_cast<int>(m_state);
    const MugKeyframe &from = Keyframes[stage - 1];
    const MugKeyframe &to = Keyframes[stage];

    m_timer += deltaTime;
    float t = glm::clamp(m_timer / getDuration(), 0.0f, 1.0f);
    float lift = glm::mix(from.lift, to.lift, ease(t));
    float tilt = glm::mix(from.tilt, to.tilt, ease(t));

    if (m_state == MugState::Drinking) {
        m_drop = glm::mix(m_dropFrom, m_dropTo, t);
        updateCoffee(context.scene);
    }

    float angle = tilt * (m_isLastSip ? LastTiltAngle : TiltAngle);
    glm::vec3 position = glm::mix(m_mugRest, m_mugHold, lift);
    setTransform(context.scene, glm::translate(position) * glm::rotate(angle, m_tiltAxis) * glm::translate(-m_mugRest));

    if (t < 1.0f) {
        return true;
    }

    m_timer = 0.0f;
    if (m_state == MugState::Lowering) {
        m_state = MugState::Idle;
        setTransform(context.scene, glm::mat4(1.0f));
        return false;
    }
    m_state = static_cast<MugState>(stage + 1);
    return true;
}

float Mug::getDuration() const {
    if (m_state == MugState::Drinking && m_isLastSip) {
        return LastDrinkTime;
    }
    return Keyframes[static_cast<int>(m_state)].duration;
}

void Mug::updateCoffee(Scene &scene) {
    if (m_coffee < 0) {
        return;
    }
    setPartBase(scene, m_coffee, glm::translate(glm::vec3(0.0f, -m_drop, 0.0f)) * m_coffeeFull);
    setPartVisible(scene, m_coffee, m_drop < static_cast<float>(FullSips) * SipDrop - 0.0001f);
}

const std::string &Mug::getIcon() const {
    if (m_state != MugState::Idle || m_sips > 0) {
        return CoffeeIcon;
    }
    return RefillIcon;
}
