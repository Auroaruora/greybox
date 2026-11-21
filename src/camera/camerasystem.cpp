#include "camerasystem.h"

#include "screen/scene.h"

void CameraSystem::add(std::unique_ptr<CameraController> controller) {
    CameraController *added = controller.get();
    if (m_controller == nullptr) {
        m_controller = added;
    }
    m_controllers[added->getName()] = std::move(controller);
}

void CameraSystem::load(const Scene &scene) {
    const SceneCameraData &data = scene.getCameraData();
    m_startPose.position = glm::vec3(data.pos);
    m_startPose.look = glm::vec3(data.look);
    m_startPose.up = glm::vec3(data.up);
    m_startPose.heightAngle = data.heightAngle;

    for (auto &[name, controller] : m_controllers) {
        controller->load(scene.getViewpoints());
    }
    reset();
}

void CameraSystem::reset() {
    setCamera(m_startPose);
    for (auto &[name, controller] : m_controllers) {
        controller->reset(m_camera);
    }
}

void CameraSystem::update(float deltaTime) {
    if (m_controller != nullptr) {
        m_controller->update(m_camera, deltaTime);
    }
}

void CameraSystem::setController(const std::string &name, const ControllerEntry &entry) {
    auto found = m_controllers.find(name);
    if (found == m_controllers.end()) {
        return;
    }
    CameraController *controller = found->second.get();
    if (isBlending() || controller == m_controller) {
        return;
    }
    m_controller = controller;

    m_controller->enter(m_camera);
}

void CameraSystem::setCamera(const CameraPose &pose) {
    m_camera.setPose(pose);
}

void CameraSystem::setAspectRatio(float aspectRatio) {
    m_camera.setAspectRatio(aspectRatio);
}

const Camera &CameraSystem::getCamera() const {
    return m_camera;
}

const std::string &CameraSystem::getControllerName() const {
    static const std::string None;
    return m_controller != nullptr ? m_controller->getName() : None;
}

std::vector<std::string> CameraSystem::getControllerNames() const {
    std::vector<std::string> names;
    for (const auto &[name, controller] : m_controllers) {
        names.push_back(name);
    }
    return names;
}

bool CameraSystem::isBlending() const {
    return false;
}
