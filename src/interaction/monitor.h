#pragma once

#include "interaction/interactable.h"

class Monitor : public Interactable {
public:
    Monitor(const std::string &name, const Texture2D &screenTexture);

    void load(Scene &scene, MeshCache &meshCache) override;

private:
    const Texture2D &m_screenTexture;
};
