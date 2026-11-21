#pragma once

#include "actions.h"

#include <glm/glm.hpp>

#include <unordered_map>
#include <vector>

struct GLFWwindow;

enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
};

class Input {
public:
    static void initialize(GLFWwindow *window);
    static void endFrame();
    static void setBlocked(bool mouse, bool keyboard);

    static void bind(Action action, int key);
    static void bind(Action action, MouseButton button);

    static bool getButton(Action action);
    static bool getButtonDown(Action action);
    static float getAxis(Action positive, Action negative);

    static glm::vec2 getMousePosition();
    static glm::vec2 getMouseDelta();
    static glm::vec2 getScrollDelta();

private:
    static void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void onMouseButton(GLFWwindow *window, int button, int action, int mods);
    static void onMouseMove(GLFWwindow *window, double x, double y);
    static void onScroll(GLFWwindow *window, double x, double y);

    static std::unordered_map<Action, std::vector<int>> s_keyBindings;
    static std::unordered_map<Action, std::vector<int>> s_mouseBindings;
    static std::unordered_map<int, bool> s_keyState;
    static std::unordered_map<int, bool> s_keyStateDown;
    static std::unordered_map<int, bool> s_mouseButtonState;
    static std::unordered_map<int, bool> s_mouseButtonStateDown;

    static glm::vec2 s_mousePosition;
    static glm::vec2 s_mouseDelta;
    static glm::vec2 s_scrollDelta;
    static bool s_firstMouseInput;
    static bool s_mouseBlocked;
    static bool s_keyboardBlocked;
};