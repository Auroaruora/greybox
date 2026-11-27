#pragma once

#include "utils/sceneparser.h"

#include <string>
#include <vector>

class MeshCache;
class TextureCache;

class Scene {
public:
    bool load(const std::string &filepath, MeshCache &meshCache);
    void loadMeshes(MeshCache &meshCache);
    void loadTextures(TextureCache &textureCache);
    void setLightEnabled(int index, bool enabled);
    void setLighting(const std::string &name);
    void setShapeTransform(int index, const glm::mat4 &ctm);
    void setShapeVisible(int index, bool visible);
    void setShapeEmissiveMap(int index, const Texture2D *texture);
    int getLightIndex(const std::string &name) const;

    const std::string &getFilepath() const;
    const RenderData &getRenderData() const;
    const SceneCameraData &getCameraData() const;
    const std::vector<SceneLightData> &getLights() const;
    const std::vector<RenderShapeData> &getShapes() const;
    const std::vector<LightingState> &getLightingStates() const;
    const std::string &getLightingName() const;
    const std::vector<Viewpoint> &getViewpoints() const;
    std::string getSummary() const;

private:
    RenderData m_data;
    std::string m_filepath;
    std::vector<LightingState> m_lightingStates;
    std::string m_lightingName;
    std::vector<Viewpoint> m_viewpoints;
};
