#include "lightingpanel.h"

#include "debug/debugcontext.h"
#include "screen/screen.h"

#include <imgui.h>

#include <string>
#include <vector>

static int getEnabledLightCount(const std::vector<SceneLightData> &lights) {
    int count = 0;
    for (const SceneLightData &light : lights) {
        if (light.enabled) {
            count++;
        }
    }
    return count;
}

static const char *getLightTypeName(LightType type) {
    switch (type) {
    case LightType::LIGHT_POINT:
        return "point";
    case LightType::LIGHT_DIRECTIONAL:
        return "directional";
    case LightType::LIGHT_SPOT:
        return "spot";
    default:
        return "light";
    }
}

static std::string getLightLabel(const SceneLightData &light, int index) {
    std::string type = getLightTypeName(light.type);
    if (light.name.empty()) {
        return std::to_string(index) + " " + type;
    }
    return light.name + " (" + type + ")###light" + std::to_string(index);
}

static void drawLighting(Screen &screen) {
    const std::vector<LightingGroup> &groups = screen.getScene().getLightingGroups();
    if (groups.empty()) {
        return;
    }

    std::string title = "Lighting (" + std::to_string(groups.size()) + ")###Lighting";
    if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    for (int i = 0; i < static_cast<int>(groups.size()); i++) {
        if (ImGui::Selectable(groups[i].name.c_str(), groups[i].name == screen.getScene().getLightingName())) {
            screen.getScene().setLighting(groups[i].name);
        }
    }
}

static void drawLights(Screen &screen) {
    const std::vector<SceneLightData> &lights = screen.getScene().getLights();
    if (lights.empty()) {
        return;
    }

    int total = static_cast<int>(lights.size());
    int enabledCount = getEnabledLightCount(lights);
    std::string title = "Lights (" + std::to_string(enabledCount) + "/" + std::to_string(total) + ")###Lights";
    if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    bool all = enabledCount == total;
    if (ImGui::Checkbox("All", &all)) {
        for (int i = 0; i < total; i++) {
            screen.getScene().setLightEnabled(i, all);
        }
    }

    for (int i = 0; i < total; i++) {
        std::string label = getLightLabel(lights[i], i);
        bool enabled = lights[i].enabled;
        if (ImGui::Checkbox(label.c_str(), &enabled)) {
            screen.getScene().setLightEnabled(i, enabled);
        }
    }
}

namespace LightingPanel {

void draw(DebugContext &context) {
    Screen &screen = *context.screens[context.current];
    drawLighting(screen);
    drawLights(screen);
}

}
