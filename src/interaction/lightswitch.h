#pragma once

#include "interaction/interactable.h"

class LightSwitch : public Interactable {
public:
    LightSwitch(const std::string &name, const std::string &light);

    void interact(InteractionContext &context) override;

private:
    std::string m_light;
};
