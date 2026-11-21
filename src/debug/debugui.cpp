#include "debugui.h"

#include "capture.h"
#include "debug.h"
#include "debuglayout.h"
#include "input/input.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <string>

static bool s_visible = true;

namespace DebugUI {

void initialize(GLFWwindow *window) {
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void update(Screen &screen) {
    ImGuiIO &io = ImGui::GetIO();
    Input::setBlocked(io.WantCaptureMouse, io.WantCaptureKeyboard);

    if (Input::getButtonDown(Action::DebugToggleOverlay)) {
        s_visible = !s_visible;
    }
    Capture::update(screen);
}

void render(const std::vector<std::unique_ptr<Screen>> &screens, int &current, Cursor &cursor, SceneRenderer &sceneRenderer, const Desktop &desktop) {
    Capture::save();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (s_visible) {
        DebugLayout::draw(screens, current, cursor, sceneRenderer, desktop);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}
