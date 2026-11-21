#include "pathplayer.h"

#include "camera/camera.h"

#include <algorithm>
#include <utility>

void PathPlayer::play(CameraPath path) {
    m_path = std::move(path);
    m_time = 0.0f;
    m_playing = true;
}

void PathPlayer::update(Camera &camera, float deltaTime) {
    if (!m_playing) {
        return;
    }
    m_time = std::min(m_time + deltaTime, m_path.getDuration());
    camera.setPose(m_path.getPose(m_time));
    m_playing = m_time < m_path.getDuration();
}

bool PathPlayer::isPlaying() const {
    return m_playing;
}
