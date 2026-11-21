#pragma once

#include <memory>
#include <vector>

class Cursor;
class SceneRenderer;
class Screen;

struct DebugContext {
    const std::vector<std::unique_ptr<Screen>> &screens;
    int &current;
    Cursor &cursor;
    SceneRenderer &sceneRenderer;
};
