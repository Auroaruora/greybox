#pragma once

#include "camera/camera.h"
#include "camera/cameracontroller.h"
#include "camera/pathplayer.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Scene;

class CameraSystem {
public:
    void add(std::unique_ptr<CameraController> controller);
    void load(const Scene &scene);
    void update(float deltaTime);
    void setController(const std::string &name, float transitionTime = 0.0f);
    void setCamera(const CameraPose &pose);
    void setAspectRatio(float aspectRatio);

    const Camera &getCamera() const;
    const std::string &getControllerName() const;
    std::vector<std::string> getControllerNames() const;
    bool isTransitioning() const;

private:
    void reset();

    Camera m_camera;
    CameraPose m_startPose;
    std::map<std::string, std::unique_ptr<CameraController>> m_controllers;
    CameraController *m_controller = nullptr;
    PathPlayer m_pathPlayer;
};
