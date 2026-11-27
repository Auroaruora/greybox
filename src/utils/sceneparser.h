#pragma once

#include "camera/camerapose.h"
#include "scenedata.h"
#include <functional>
#include <map>
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

struct LightingGroup {
    std::string name;
    glm::vec3 ambient;
    std::vector<SceneLightData> lights;
    std::map<std::string, SceneColor> emissive;
};

struct Viewpoint {
    std::string name;
    CameraPose pose;
};

class SceneParser {
public:
    using MaterialLookup = std::function<std::string(const std::string &meshfile)>;

    // Parse a scene from its look file and the files next to it, and store the results.
    // @param lookFile        The look to load, e.g. scenes/room/looks/greybox.json.
    // @param getMeshMaterial Returns the default material name of a mesh file.
    // @param renderData      On return, this will contain the metadata of the loaded scene.
    // @param lightingGroups  On return, one group per lighting state, the start state first.
    // @param viewpoints      On return, the scene's named camera poses.
    // @return                A boolean value indicating whether the parse was successful.
    static bool parse(const std::string &lookFile, const MaterialLookup &getMeshMaterial, RenderData &renderData,
                      std::vector<LightingGroup> &lightingGroups, std::vector<Viewpoint> &viewpoints);
};
