#pragma once

#include "window.h"
#include "debug/debugcontext.h"
#include "gl/meshcache.h"
#include "gl/texturecache.h"
#include "input/cursor.h"
#include "render/scenerenderer.h"
#include "render/spriterenderer.h"
#include "screen/screen.h"

#include <memory>
#include <vector>

class App {
public:
    bool initialize();
    void run();
    void shutdown();
    void setScreen(const std::string &name);

private:
    void update(float deltaTime);
    void render();
    void addScreen(std::unique_ptr<Screen> screen);

    Window m_window;
    MeshCache m_meshCache;
    TextureCache m_textureCache;
    SceneRenderer m_sceneRenderer;
    SpriteRenderer m_spriteRenderer;
    Cursor m_cursor;
    std::vector<std::unique_ptr<Screen>> m_screens;
    int m_current = 0;
    DebugContext m_debugContext{m_screens, m_current, m_cursor, m_sceneRenderer};
};
