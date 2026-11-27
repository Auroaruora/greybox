#pragma once

#include "camera/camerasystem.h"
#include "interaction/interactionsystem.h"
#include "render/scenerenderer.h"
#include "screen/motions.h"
#include "screen/scene.h"

#include <memory>
#include <string>

class App;
class TextureCache;

class Screen {
public:
    Screen(const std::string &name, const std::string &sceneFile);
    virtual ~Screen() = default;

    bool load(MeshCache &meshCache, TextureCache &textureCache);
    virtual void enter();
    void update(App &app, float deltaTime, const glm::vec2 &cursorNdc, float aspectRatio);
    void render(SceneRenderer &sceneRenderer) const;

    const std::string &getName() const;
    const std::string &getSceneFile() const;
    Scene &getScene();
    const Scene &getScene() const;
    CameraSystem &getCameraSystem();
    const CameraSystem &getCameraSystem() const;
    const InteractionSystem &getInteractionSystem() const;

protected:
    virtual void handleEvents(App &app);

    Scene m_scene;
    Motions m_motions;
    CameraSystem m_cameraSystem;
    InteractionSystem m_interactionSystem;

private:
    std::string m_name;
    std::string m_sceneFile;
};
