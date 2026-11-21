#pragma once

#include <memory>
#include <vector>

class Cursor;
class Desktop;
class SceneRenderer;
class Screen;

namespace DebugLayout {
void draw(const std::vector<std::unique_ptr<Screen>> &screens, int &current, Cursor &cursor, SceneRenderer &sceneRenderer, const Desktop &desktop);
}
