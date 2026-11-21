#include "screenspanel.h"

#include "debug/debugcontext.h"
#include "screen/screen.h"

#include <imgui.h>

namespace ScreensPanel {

void draw(DebugContext &context) {
    if (!ImGui::CollapsingHeader("Screens", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    for (int i = 0; i < static_cast<int>(context.screens.size()); i++) {
        if (ImGui::Selectable(context.screens[i]->getName().c_str(), i == context.current)) {
            context.current = i;
        }
    }
}

}
