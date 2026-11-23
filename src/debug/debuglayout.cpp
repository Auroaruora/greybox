#include "debuglayout.h"

#include "capture.h"
#include "debugcontext.h"
#include "debug/panels/camerapanel.h"
#include "debug/panels/infopanel.h"
#include "debug/panels/lightingpanel.h"
#include "debug/panels/postprocessingpanel.h"
#include "debug/panels/screenspanel.h"
#include "screen/screen.h"

#include <imgui.h>

#include <algorithm>
#include <string>

enum class PanelSide {
    Left,
    Right,
};

static const float PanelWidth = 340.0f;
static const float MinPanelWidth = 240.0f;

static PanelSide s_panelSide = PanelSide::Right;

static void placePanel() {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    bool isRight = s_panelSide == PanelSide::Right;
    float x = isRight ? viewport->WorkPos.x + viewport->WorkSize.x : viewport->WorkPos.x;
    float maxWidth = std::max(MinPanelWidth, viewport->WorkSize.x * 0.5f);

    ImGui::SetNextWindowPos(ImVec2(x, viewport->WorkPos.y), ImGuiCond_Always, ImVec2(isRight ? 1.0f : 0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, viewport->WorkSize.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(MinPanelWidth, viewport->WorkSize.y), ImVec2(maxWidth, viewport->WorkSize.y));
}

static void drawPanelSide() {
    int side = static_cast<int>(s_panelSide);
    ImGui::RadioButton("Left", &side, static_cast<int>(PanelSide::Left));
    ImGui::SameLine();
    ImGui::RadioButton("Right", &side, static_cast<int>(PanelSide::Right));
    s_panelSide = static_cast<PanelSide>(side);
}

static void drawCapture(Screen &screen) {
    ImGui::SameLine();
    ImGui::BeginDisabled(Capture::isBatchActive());
    if (ImGui::Button("Screenshot")) {
        Capture::request(screen, CameraPanel::getActiveViewpoint(screen));
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

namespace DebugLayout {

void draw(DebugContext &context) {
    Screen &screen = *context.screens[context.current];

    placePanel();
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    drawPanelSide();
    drawCapture(screen);
    InfoPanel::draw(context);
    ScreensPanel::draw(context);
    CameraPanel::draw(context);
    LightingPanel::draw(context);
    PostProcessingPanel::draw(context);
    ImGui::End();
}

}
