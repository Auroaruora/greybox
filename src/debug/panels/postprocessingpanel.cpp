#include "postprocessingpanel.h"

#include "debug/debugcontext.h"
#include "render/scenerenderer.h"

#include <imgui.h>

#include <memory>
#include <string>
#include <vector>

namespace PostProcessingPanel {

void draw(DebugContext &context) {
    PostProcessor &postProcessor = context.sceneRenderer.getPostProcessor();
    const std::vector<std::unique_ptr<Pass>> &passes = postProcessor.getPasses();
    std::string title = "Post processing (" + std::to_string(passes.size()) + ")###PostProcessing";
    if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    int from = -1;
    int to = -1;
    for (int i = 0; i < static_cast<int>(passes.size()); i++) {
        Pass &pass = *passes[i];
        ImGui::PushID(i);
        if (ImGui::ArrowButton("up", ImGuiDir_Up)) {
            from = i;
            to = i - 1;
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("down", ImGuiDir_Down)) {
            from = i;
            to = i + 1;
        }
        ImGui::SameLine();
        ImGui::Checkbox(pass.name.c_str(), &pass.isEnabled);
        ImGui::PopID();
    }
    postProcessor.movePass(from, to);
}

}
