#pragma once

#include "interaction/interactable.h"

class Monitor : public Interactable {
public:
    Monitor(const std::string &name, const std::string &imageFile);

    void load(Scene &scene, MeshCache &meshCache, TextureCache &textureCache) override;

private:
    std::string m_imageFile;
};
