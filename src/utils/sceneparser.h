#pragma once

#include "scenedata.h"
#include <vector>
#include <string>

class Mesh;
class Texture2D;

// Struct which contains data for a single primitive, to be used for rendering
struct RenderShapeData {
    ScenePrimitive primitive;
    glm::mat4 ctm; // the cumulative transformation matrix
    glm::mat4 inverseCtm;
    glm::mat3 normalMatrix;
    const Mesh *mesh = nullptr;
    const Texture2D *textureMap = nullptr;
    const Texture2D *emissiveMap = nullptr;
    bool visible = true;
    bool emissiveEnabled = true;
};

// Struct which contains all the data needed to render a scene
struct RenderData {
    SceneGlobalData globalData;
    SceneCameraData cameraData;

    std::vector<SceneLightData> lights;
    std::vector<RenderShapeData> shapes;
};

class SceneParser {
public:
    // Parse the scene and store the results in renderData.
    // @param filepath    The path of the scene file to load.
    // @param renderData  On return, this will contain the metadata of the loaded scene.
    // @return            A boolean value indicating whether the parse was successful.
    static bool parse(const std::string &filepath, RenderData &renderData);
};
