#pragma once

#include "scenedata.h"

#include <string>
#include <vector>

class ScenefileReader {
public:
    ScenefileReader(const std::string &filename);
    ~ScenefileReader();
    ScenefileReader(const ScenefileReader &) = delete;
    ScenefileReader &operator=(const ScenefileReader &) = delete;

    bool readJSON();

    SceneGlobalData getGlobalData() const;
    SceneCameraData getCameraData() const;
    SceneNode *getRootNode() const;

private:
    SceneNode *addNode(SceneNode *parent);

    std::string m_filename;
    SceneGlobalData m_globalData = {};
    SceneCameraData m_cameraData = {};
    SceneNode *m_root = nullptr;
    std::vector<SceneNode *> m_nodes;
};