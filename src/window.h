#pragma once

#include <glm/glm.hpp>

#include <string>

struct GLFWwindow;

class Window {
public:
    Window() = default;
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    bool create(int width, int height, const std::string &title);
    void destroy();

    bool isOpen() const;

    void pollEvents();
    void swapBuffers();
    void bind() const;

    glm::ivec2 getSize() const;
    glm::ivec2 getFramebufferSize() const;
    float getAspectRatio() const;

    double getTime() const;
    GLFWwindow *getHandle() const;

private:
    GLFWwindow *m_window = nullptr;
};
