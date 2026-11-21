#include "camerapanel.h"

#include "debug/debugcontext.h"
#include "screen/screen.h"

#include <glm/glm.hpp>
#include <imgui.h>

#include <string>
#include <vector>

static const std::string DebugCamera = "debug";
static const float DebugTransitionTime = 1.0f;

static std::string s_viewpointScene;
static int s_selectedViewpoint = -1;

static void syncScene(const Screen &screen) {
    if (s_viewpointScene == screen.getSceneFile()) {
        return;
    }
    s_viewpointScene = screen.getSceneFile();
    s_selectedViewpoint = -1;
}

static bool isAtViewpoint(const Camera &camera, const Viewpoint &viewpoint) {
    const float tolerance = 0.001f;
    const CameraPose &pose = camera.getPose();
    return glm::distance(pose.position, viewpoint.pose.position) < tolerance
        && glm::distance(pose.look, glm::normalize(viewpoint.pose.look)) < tolerance;
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
            screen.getCameraSystem().setController(name, DebugTransitionTime);
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

    int active = CameraPanel::getActiveViewpoint(screen);
    for (int i = 0; i < static_cast<int>(viewpoints.size()); i++) {
        const Viewpoint &viewpoint = viewpoints[i];
        if (ImGui::Selectable(viewpoint.name.c_str(), i == active)) {
            screen.getCameraSystem().setController(DebugCamera);
            screen.getCameraSystem().setCamera(viewpoint.pose);
            s_selectedViewpoint = i;
        }
    }
}

namespace CameraPanel {

void draw(DebugContext &context) {
    Screen &screen = *context.screens[context.current];
    drawControllers(screen);
    drawViewpoints(screen);
}

int getActiveViewpoint(const Screen &screen) {
    syncScene(screen);
    if (s_selectedViewpoint < 0 || !isAtViewpoint(screen.getCameraSystem().getCamera(), screen.getScene().getViewpoints()[s_selectedViewpoint])) {
        return -1;
    }
    return s_selectedViewpoint;
}

}
