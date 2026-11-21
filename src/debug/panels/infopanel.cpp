#include "infopanel.h"

#include "debug/debugcontext.h"
#include "input/cursor.h"
#include "screen/screen.h"

#include <glm/glm.hpp>
#include <imgui.h>

static void drawTiming() {
    const ImGuiIO &io = ImGui::GetIO();
    ImGui::Text("%.1f frames per second, %.2f ms per frame", io.Framerate, 1000.0f / io.Framerate);
}

static void drawDisplay(const Cursor &cursor, const Camera &camera) {
    const ImGuiIO &io = ImGui::GetIO();
    glm::vec2 position = cursor.getPosition();
    glm::vec2 ndc = cursor.getNdc();
    const CameraPose &pose = camera.getPose();
    ImGui::Text("window       %.0f x %.0f points", io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("framebuffer  %.0f x %.0f pixels (%.2fx)", io.DisplaySize.x * io.DisplayFramebufferScale.x, io.DisplaySize.y * io.DisplayFramebufferScale.y, io.DisplayFramebufferScale.x);
    ImGui::Text("mouse        %.1f, %.1f points", position.x, position.y);
    ImGui::Text("cursor ndc   %.2f, %.2f", ndc.x, ndc.y);
    ImGui::Text("camera pos   %.2f  %.2f  %.2f", pose.position.x, pose.position.y, pose.position.z);
    ImGui::Text("camera look  %.2f  %.2f  %.2f", pose.look.x, pose.look.y, pose.look.z);
}

namespace InfoPanel {

void draw(DebugContext &context) {
    const Screen &screen = *context.screens[context.current];
    drawTiming();
    drawDisplay(context.cursor, screen.getCameraSystem().getCamera());
}

}
