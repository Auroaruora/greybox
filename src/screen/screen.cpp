#include "screen.h"

#include <filesystem>
#include <optional>

Screen::Screen(const std::string &name, const std::string &sceneFile) {
    m_name = name;
    m_sceneFile = sceneFile;
}

bool Screen::load(MeshCache &meshCache, TextureCache &textureCache) {
    if (!m_scene.load(m_sceneFile, meshCache)) {
        return false;
    }
    m_scene.loadMeshes(meshCache);
    m_scene.loadTextures(textureCache);
    m_cameraSystem.load(m_scene);
    m_interactionSystem.load(m_scene, meshCache, textureCache);
    std::filesystem::path folder = std::filesystem::path(m_sceneFile).parent_path().parent_path();
    m_motions.load(m_scene, meshCache, (folder / "motions.json").string());
    return true;
}

void Screen::enter() {
}

void Screen::handleEvents(App &app) {
}

void Screen::update(App &app, float deltaTime, const glm::vec2 &cursorNdc, float aspectRatio) {
    m_motions.update(m_scene, deltaTime);
    m_cameraSystem.setAspectRatio(aspectRatio);
    m_cameraSystem.update(deltaTime);

    const Camera &camera = m_cameraSystem.getCamera();
    std::optional<Ray> pointer;
    if (!m_cameraSystem.isTransitioning()) {
        pointer = camera.getRayThrough(cursorNdc);
    }
    m_interactionSystem.update(m_scene, camera, pointer, deltaTime);

    handleEvents(app);
}

void Screen::render(SceneRenderer &sceneRenderer) const {
    sceneRenderer.render(m_cameraSystem.getCamera(), m_scene.getRenderData());
}

const std::string &Screen::getName() const {
    return m_name;
}

const std::string &Screen::getSceneFile() const {
    return m_sceneFile;
}

Scene &Screen::getScene() {
    return m_scene;
}

const Scene &Screen::getScene() const {
    return m_scene;
}

CameraSystem &Screen::getCameraSystem() {
    return m_cameraSystem;
}

const CameraSystem &Screen::getCameraSystem() const {
    return m_cameraSystem;
}

const InteractionSystem &Screen::getInteractionSystem() const {
    return m_interactionSystem;
}
