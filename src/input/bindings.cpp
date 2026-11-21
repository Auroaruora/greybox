#include "bindings.h"

#include "debug/debug.h"
#include "input/input.h"

#include <GLFW/glfw3.h>

namespace Bindings {

void setDefaults() {
    Input::bind(Action::LookLeft, GLFW_KEY_A);
    Input::bind(Action::LookLeft, GLFW_KEY_LEFT);
    Input::bind(Action::LookRight, GLFW_KEY_D);
    Input::bind(Action::LookRight, GLFW_KEY_RIGHT);
    Input::bind(Action::LookUp, GLFW_KEY_W);
    Input::bind(Action::LookUp, GLFW_KEY_UP);
    Input::bind(Action::LookDown, GLFW_KEY_S);
    Input::bind(Action::LookDown, GLFW_KEY_DOWN);
    Input::bind(Action::Back, GLFW_KEY_ESCAPE);
    Input::bind(Action::Interact, MouseButton::Left);

    if (!debug::ToolsEnabled) {
        return;
    }
    Input::bind(Action::DebugMoveForward, GLFW_KEY_W);
    Input::bind(Action::DebugMoveBack, GLFW_KEY_S);
    Input::bind(Action::DebugMoveLeft, GLFW_KEY_A);
    Input::bind(Action::DebugMoveRight, GLFW_KEY_D);
    Input::bind(Action::DebugMoveUp, GLFW_KEY_E);
    Input::bind(Action::DebugMoveDown, GLFW_KEY_Q);
    Input::bind(Action::DebugMoveFast, GLFW_KEY_LEFT_SHIFT);
    Input::bind(Action::DebugViewPan, GLFW_KEY_LEFT_SHIFT);
    Input::bind(Action::DebugViewPan, GLFW_KEY_RIGHT_SHIFT);
    Input::bind(Action::DebugViewZoom, GLFW_KEY_LEFT_CONTROL);
    Input::bind(Action::DebugViewZoom, GLFW_KEY_RIGHT_CONTROL);
    Input::bind(Action::DebugViewDrag, MouseButton::Middle);
    Input::bind(Action::DebugViewEmulate, GLFW_KEY_LEFT_ALT);
    Input::bind(Action::DebugViewEmulate, GLFW_KEY_RIGHT_ALT);
    Input::bind(Action::DebugViewEmulateDrag, MouseButton::Left);
    Input::bind(Action::DebugToggleOverlay, GLFW_KEY_GRAVE_ACCENT);
}

}
