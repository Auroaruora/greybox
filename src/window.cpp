#include "window.h"

#include "debug/debug.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

static void onGlfwError(int code, const char *description) {
    LOG_ERROR("GLFW error " << code << ": " << description);
}

bool Window::create(int width, int height, const std::string &title) {
    glfwSetErrorCallback(onGlfwError);
    if (!glfwInit()) {
        LOG_ERROR("could not initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (m_window == nullptr) {
        LOG_ERROR("could not create the window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    glewExperimental = GL_TRUE;
    GLenum error = glewInit();
    if (error != GLEW_OK) {
        LOG_ERROR("could not initialize GLEW: " << glewGetErrorString(error));
        destroy();
        return false;
    }

    LOG_INFO("OpenGL " << glGetString(GL_VERSION) << " on " << glGetString(GL_RENDERER));
    return true;
}

void Window::destroy() {
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

bool Window::isOpen() const {
    return m_window != nullptr && !glfwWindowShouldClose(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::bind() const {
    glm::ivec2 size = getFramebufferSize();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, size.x, size.y);
}

glm::ivec2 Window::getSize() const {
    glm::ivec2 size(0);
    if (m_window != nullptr) {
        glfwGetWindowSize(m_window, &size.x, &size.y);
    }
    return size;
}

glm::ivec2 Window::getFramebufferSize() const {
    glm::ivec2 size(0);
    if (m_window != nullptr) {
        glfwGetFramebufferSize(m_window, &size.x, &size.y);
    }
    return size;
}

float Window::getAspectRatio() const {
    glm::ivec2 size = getFramebufferSize();
    if (size.y <= 0) {
        return 1.0f;
    }
    return static_cast<float>(size.x) / static_cast<float>(size.y);
}

double Window::getTime() const {
    return glfwGetTime();
}

GLFWwindow *Window::getHandle() const {
    return m_window;
}
