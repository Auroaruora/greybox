#include "debuglayout.h"

#include "capture.h"
#include "desktop/desktop.h"
#include "input/cursor.h"
#include "screen/screen.h"

#include <glm/glm.hpp>
#include <imgui.h>

#include <string>
#include <vector>

static const std::string DebugCamera = "debug";
static const ControllerEntry DebugEntry = {};

static std::string s_viewpointScene;
static int s_selectedViewpoint = -1;

static void resetViewpoints(const Screen &screen) {
    s_viewpointScene = screen.getSceneFile();
    s_selectedViewpoint = -1;
}

static bool isAtViewpoint(const Camera &camera, const Viewpoint &viewpoint) {
    const float tolerance = 0.001f;
    const CameraPose &pose = camera.getPose();
    return glm::distance(pose.position, viewpoint.pose.position) < tolerance
        && glm::distance(pose.look, glm::normalize(viewpoint.pose.look)) < tolerance;
}

static int getActiveViewpoint(const Screen &screen) {
    if (s_selectedViewpoint < 0 || !isAtViewpoint(screen.getCameraSystem().getCamera(), screen.getScene().getViewpoints()[s_selectedViewpoint])) {
        return -1;
    }
    return s_selectedViewpoint;
}

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

static void drawTiming() {
    const ImGuiIO &io = ImGui::GetIO();
    ImGui::Text("%.1f frames per second, %.2f ms per frame", io.Framerate, 1000.0f / io.Framerate);
}

static void drawDisplay(const Cursor &cursor) {
    const ImGuiIO &io = ImGui::GetIO();
    glm::vec2 position = cursor.getPosition();
    glm::vec2 ndc = cursor.getNdc();
    ImGui::Text("window       %.0f x %.0f points", io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("framebuffer  %.0f x %.0f pixels (%.2fx)", io.DisplaySize.x * io.DisplayFramebufferScale.x, io.DisplaySize.y * io.DisplayFramebufferScale.y, io.DisplayFramebufferScale.x);
    ImGui::Text("mouse        %.1f, %.1f points", position.x, position.y);
    ImGui::Text("cursor ndc   %.2f, %.2f", ndc.x, ndc.y);
}

static void drawScreenList(const std::vector<std::unique_ptr<Screen>> &screens, int &current) {
    if (ImGui::CollapsingHeader("Screens", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < static_cast<int>(screens.size()); i++) {
            if (ImGui::Selectable(screens[i]->getName().c_str(), i == current)) {
                current = i;
            }
        }
    }
}

static void drawScreenInfo(Screen &screen) {
    if (!ImGui::CollapsingHeader("Screen info", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    const CameraPose &pose = screen.getCameraSystem().getCamera().getPose();
    const glm::vec3 &position = pose.position;
    const glm::vec3 &look = pose.look;
    ImGui::Text("%s", screen.getSceneFile().c_str());
    ImGui::Text("%d shapes", static_cast<int>(screen.getScene().getShapes().size()));
    ImGui::Text("camera position  %.2f  %.2f  %.2f", position.x, position.y, position.z);
    ImGui::Text("camera look      %.2f  %.2f  %.2f", look.x, look.y, look.z);

    if (ImGui::Button("Reset camera")) {
        screen.getCameraSystem().reset();
    }

    ImGui::BeginDisabled(Capture::isBatchActive());
    if (ImGui::Button("Screenshot")) {
        Capture::request(screen, getActiveViewpoint(screen));
    }
    if (!screen.getScene().getViewpoints().empty()) {
        ImGui::SameLine();
        if (ImGui::Button("Capture all")) {
            Capture::startBatch(screen);
        }
    }
    ImGui::EndDisabled();

    if (Capture::isBatchActive()) {
        ImGui::Text("capturing %d/%d", Capture::getBatchStep(), Capture::getBatchTotal(screen));
    }

    const std::string &capturePath = Capture::getLastPath();
    if (!capturePath.empty()) {
        ImGui::TextWrapped("saved %s", capturePath.c_str());
    }
}

static void drawControllers(Screen &screen) {
    std::vector<std::string> names = screen.getCameraSystem().getControllerNames();
    if (names.empty()) {
        return;
    }

    std::string title = "controller (" + std::to_string(names.size()) + ")###controller";
    if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    for (const std::string &name : names) {
        std::string label = name + "###controller_" + name;
        if (ImGui::Selectable(label.c_str(), name == screen.getCameraSystem().getControllerName()) && name != screen.getCameraSystem().getControllerName()) {
            screen.getCameraSystem().setController(name, DebugEntry);
        }
    }
}

static void drawViewpoints(Screen &screen) {
    const std::vector<Viewpoint> &viewpoints = screen.getScene().getViewpoints();
    if (viewpoints.empty()) {
        return;
    }

    std::string title = "Viewpoints (" + std::to_string(viewpoints.size()) + ")###Viewpoints";
    if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    int active = getActiveViewpoint(screen);
    for (int i = 0; i < static_cast<int>(viewpoints.size()); i++) {
        const Viewpoint &viewpoint = viewpoints[i];
        if (ImGui::Selectable(viewpoint.name.c_str(), i == active)) {
            screen.getCameraSystem().setController(DebugCamera);
            screen.getCameraSystem().setCamera(viewpoint.pose);
            s_selectedViewpoint = i;
        }
    }
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

static void drawInteraction(const Screen &screen) {
    const InteractionSystem &system = screen.getInteractionSystem();
    const std::vector<std::unique_ptr<Interactable>> &items = system.getItems();
    std::string title = "Interaction (" + std::to_string(items.size()) + ")###Interaction";
    if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    for (const std::unique_ptr<Interactable> &item : items) {
        std::string label = item->getName() + "  [" + item->getIcon() + "]###interaction_" + item->getName();
        ImGui::Selectable(label.c_str(), item.get() == system.getHovered());
    }
}

static void drawCursor(Cursor &cursor) {
    std::vector<std::string> names = cursor.getIconNames();
    std::string title = "Cursor (" + std::to_string(names.size()) + ")###Cursor";
    if (!ImGui::CollapsingHeader(title.c_str())) {
        return;
    }
    for (const std::string &name : names) {
        std::string label = name + "###cursor_" + name;
        ImGui::Selectable(label.c_str(), name == cursor.getIcon());
    }
}

static void drawDesktop(const Desktop &desktop) {
    if (!ImGui::CollapsingHeader("Desktop")) {
        return;
    }
    const Texture2D &texture = desktop.getScreenTexture();
    float width = ImGui::GetContentRegionAvail().x;
    float height = width * static_cast<float>(texture.getHeight()) / static_cast<float>(texture.getWidth());
    ImGui::Image(static_cast<ImTextureID>(texture.getId()), ImVec2(width, height), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
}

namespace DebugLayout {

void draw(const std::vector<std::unique_ptr<Screen>> &screens, int &current, Cursor &cursor, SceneRenderer &sceneRenderer, const Desktop &desktop) {
    Screen &screen = *screens[current];
    if (s_viewpointScene != screen.getSceneFile()) {
        resetViewpoints(screen);
    }

    ImGui::Begin("Debug");
    drawDesktop(desktop);
    drawTiming();
    drawDisplay(cursor);
    drawScreenList(screens, current);
    drawScreenInfo(screen);
    drawControllers(screen);
    drawViewpoints(screen);
    drawLighting(screen);
    drawLights(screen);
    drawInteraction(screen);
    drawCursor(cursor);
    ImGui::End();
}

}
