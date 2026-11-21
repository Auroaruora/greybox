#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;
class Cursor;
class Desktop;
class SceneRenderer;
class Screen;

namespace DebugUI {
void initialize(GLFWwindow *window);
void shutdown();

void update(Screen &screen);
void render(const std::vector<std::unique_ptr<Screen>> &screens, int &current, Cursor &cursor, SceneRenderer &sceneRenderer, const Desktop &desktop);
}
