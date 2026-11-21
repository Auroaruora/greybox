#include "monitor.h"

static const std::string Icon = "interact";
static const std::string ScreenSuffix = "_screen";

Monitor::Monitor(const std::string &name, const Texture2D &screenTexture)
    : Interactable(name, Icon), m_screenTexture(screenTexture) {
}

void Monitor::load(Scene &scene, MeshCache &meshCache) {
    Interactable::load(scene, meshCache);
    int screen = getPart(scene, getName() + ScreenSuffix);
    if (screen >= 0) {
        setPartEmissiveMap(scene, screen, &m_screenTexture);
    }
}
