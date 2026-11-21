#include "input.h"

#include <GLFW/glfw3.h>

static_assert(static_cast<int>(MouseButton::Left) == GLFW_MOUSE_BUTTON_LEFT);
static_assert(static_cast<int>(MouseButton::Right) == GLFW_MOUSE_BUTTON_RIGHT);
static_assert(static_cast<int>(MouseButton::Middle) == GLFW_MOUSE_BUTTON_MIDDLE);

static void addBinding(std::unordered_map<Action, std::vector<int>> &bindings, Action action, int code) {
    std::vector<int> &codes = bindings[action];
    for (int existing : codes) {
        if (existing == code) {
            return;
        }
    }
    codes.push_back(code);
}

static bool isPressed(const std::unordered_map<Action, std::vector<int>> &bindings, Action action, const std::unordered_map<int, bool> &states) {
    auto binding = bindings.find(action);
    if (binding == bindings.end()) {
        return false;
    }
    for (int code : binding->second) {
        auto state = states.find(code);
        if (state != states.end() && state->second) {
            return true;
        }
    }
    return false;
}

std::unordered_map<Action, std::vector<int>> Input::s_keyBindings;
std::unordered_map<Action, std::vector<int>> Input::s_mouseBindings;
std::unordered_map<int, bool> Input::s_keyState;
std::unordered_map<int, bool> Input::s_keyStateDown;
std::unordered_map<int, bool> Input::s_mouseButtonState;
std::unordered_map<int, bool> Input::s_mouseButtonStateDown;

glm::vec2 Input::s_mousePosition = glm::vec2(0.0f);
glm::vec2 Input::s_mouseDelta = glm::vec2(0.0f);
glm::vec2 Input::s_scrollDelta = glm::vec2(0.0f);
bool Input::s_firstMouseInput = true;
bool Input::s_mouseBlocked = false;
bool Input::s_keyboardBlocked = false;

void Input::initialize(GLFWwindow *window) {
    glfwSetKeyCallback(window, onKey);
    glfwSetMouseButtonCallback(window, onMouseButton);
    glfwSetCursorPosCallback(window, onMouseMove);
    glfwSetScrollCallback(window, onScroll);

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);
    s_mousePosition = glm::vec2(static_cast<float>(x), static_cast<float>(y));
}

void Input::endFrame() {
    s_mouseDelta = glm::vec2(0.0f);
    s_scrollDelta = glm::vec2(0.0f);
    s_keyStateDown.clear();
    s_mouseButtonStateDown.clear();
}

void Input::setBlocked(bool mouse, bool keyboard) {
    s_mouseBlocked = mouse;
    s_keyboardBlocked = keyboard;
}

void Input::bind(Action action, int key) {
    addBinding(s_keyBindings, action, key);
}

void Input::bind(Action action, MouseButton button) {
    addBinding(s_mouseBindings, action, static_cast<int>(button));
}

bool Input::getButton(Action action) {
    return (!s_keyboardBlocked && isPressed(s_keyBindings, action, s_keyState))
        || (!s_mouseBlocked && isPressed(s_mouseBindings, action, s_mouseButtonState));
}

bool Input::getButtonDown(Action action) {
    return (!s_keyboardBlocked && isPressed(s_keyBindings, action, s_keyStateDown))
        || (!s_mouseBlocked && isPressed(s_mouseBindings, action, s_mouseButtonStateDown));
}

float Input::getAxis(Action positive, Action negative) {
    float value = 0.0f;
    if (getButton(positive)) {
        value += 1.0f;
    }
    if (getButton(negative)) {
        value -= 1.0f;
    }
    return value;
}

glm::vec2 Input::getMousePosition() {
    return s_mousePosition;
}

glm::vec2 Input::getMouseDelta() {
    return s_mouseBlocked ? glm::vec2(0.0f) : s_mouseDelta;
}

glm::vec2 Input::getScrollDelta() {
    return s_mouseBlocked ? glm::vec2(0.0f) : s_scrollDelta;
}

void Input::onKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        s_keyState[key] = true;
        s_keyStateDown[key] = true;
    } else if (action == GLFW_RELEASE) {
        s_keyState[key] = false;
    }
}

void Input::onMouseButton(GLFWwindow *window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        s_mouseButtonState[button] = true;
        s_mouseButtonStateDown[button] = true;
    } else if (action == GLFW_RELEASE) {
        s_mouseButtonState[button] = false;
    }
}

void Input::onMouseMove(GLFWwindow *window, double x, double y) {
    glm::vec2 position(static_cast<float>(x), static_cast<float>(y));
    if (s_firstMouseInput) {
        s_mousePosition = position;
        s_firstMouseInput = false;
    }
    s_mouseDelta += position - s_mousePosition;
    s_mousePosition = position;
}

void Input::onScroll(GLFWwindow *window, double x, double y) {
    s_scrollDelta += glm::vec2(static_cast<float>(x), static_cast<float>(y));
}