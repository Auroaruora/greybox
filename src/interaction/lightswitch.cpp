#include "lightswitch.h"

#include "debug/debug.h"
#include "screen/scene.h"

static const std::string Icon = "lamp";

LightSwitch::LightSwitch(const std::string &name, const std::string &light)
    : Interactable(name, Icon) {
    m_light = light;
}

void LightSwitch::interact(InteractionContext &context) {
    int index = context.scene.getLightIndex(m_light);
    if (index < 0) {
        LOG_INFO(getName() << ": no light named " << m_light);
        return;
    }

    bool enabled = !context.scene.getLights()[index].enabled;
    context.scene.setLightEnabled(index, enabled);
    LOG_INFO(getName() << ": " << m_light << (enabled ? " on" : " off"));
}
