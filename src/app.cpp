#include "app.h"

#include "debug/debug.h"
#include "debug/debugui.h"
#include "input/bindings.h"
#include "input/input.h"
#include "screen/roomscreen.h"

static const int WindowWidth = 1280;
static const int WindowHeight = 720;

bool App::initialize() {
    if (!m_window.create(WindowWidth, WindowHeight, "Greybox")) {
        return false;
    }

    m_sceneRenderer.initialize();
    m_spriteRenderer.initialize();
    m_desktop.initialize();
    m_cursor.initialize();

    Input::initialize(m_window.getHandle());
    Bindings::setDefaults();
    if (debug::ToolsEnabled) {
        DebugUI::initialize(m_window.getHandle());
    }

    addScreen(std::make_unique<RoomScreen>("room_greybox", "scenes/room_greybox.json", m_desktop));
    if (debug::ToolsEnabled) {
        // Testing screen can be put here
    }

    return true;
}

void App::run() {
    double previousTime = m_window.getTime();

    while (m_window.isOpen()) {
        double currentTime = m_window.getTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        m_window.pollEvents();
        update(deltaTime);
        render();
        m_window.swapBuffers();
    }
}

void App::update(float deltaTime) {
    Screen &screen = *m_screens[m_current];
    if (debug::ToolsEnabled) {
        DebugUI::update(m_debugContext);
    }
    m_cursor.update(m_window.getSize());
    screen.update(deltaTime, m_cursor.getNdc(), m_window.getAspectRatio());
    m_cursor.setIcon(screen.getInteractionSystem().getIcon());
    Input::endFrame();
}

void App::render() {
    m_desktop.updateScreenTexture(m_spriteRenderer);
    m_window.bind();
    m_screens[m_current]->render(m_sceneRenderer);
    m_window.bind();
    if (debug::ToolsEnabled) {
        DebugUI::render(m_debugContext);
    }
    m_cursor.draw(m_spriteRenderer, m_window.getSize());
}

void App::shutdown() {
    if (debug::ToolsEnabled) {
        DebugUI::shutdown();
    }
    m_cursor.cleanup();
    m_desktop.cleanup();
    m_spriteRenderer.cleanup();
    m_sceneRenderer.cleanup();
    m_meshCache.clear();
    m_textureCache.clear();
    m_window.destroy();
}

void App::addScreen(std::unique_ptr<Screen> screen) {
    screen->load(m_meshCache, m_textureCache);
    m_screens.push_back(std::move(screen));
}