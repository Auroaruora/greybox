#include "debugui.h"

#include "capture.h"
#include "debug.h"
#include "debugcontext.h"
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
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void update(DebugContext &context) {
    ImGuiIO &io = ImGui::GetIO();
    Input::setBlocked(io.WantCaptureMouse, io.WantCaptureKeyboard);

    if (Input::getButtonDown(Action::DebugToggleOverlay)) {
        s_visible = !s_visible;
    }
    Capture::update(*context.screens[context.current]);
}

void render(DebugContext &context) {
    Capture::save();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (s_visible) {
        DebugLayout::draw(context);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}
