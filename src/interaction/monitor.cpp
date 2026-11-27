#include "monitor.h"

#include "gl/texturecache.h"

static const std::string Icon = "interact";
static const std::string ScreenSuffix = "_screen";

Monitor::Monitor(const std::string &name, const std::string &imageFile) : Interactable(name, Icon) {
    m_imageFile = imageFile;
}

void Monitor::load(Scene &scene, MeshCache &meshCache, TextureCache &textureCache) {
    Interactable::load(scene, meshCache, textureCache);
    int screen = getPart(scene, getName() + ScreenSuffix);
    if (screen >= 0) {
        setPartEmissiveMap(scene, screen, textureCache.getTexture(m_imageFile));
    }
}
