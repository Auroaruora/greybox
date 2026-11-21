#pragma once

#include "camera/camerapath.h"

class Camera;

class PathPlayer {
public:
    void play(CameraPath path);
    void update(Camera &camera, float deltaTime);
    bool isPlaying() const;

private:
    CameraPath m_path;
    float m_time = 0.0f;
    bool m_playing = false;
};
