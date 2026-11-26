#pragma once

#include "camera/camera.h"
#include "render/postprocess/postprocessor.h"
#include "render/shadingprogram.h"
#include "utils/sceneparser.h"

#include <map>
#include <memory>

class SceneRenderer {
public:
    void initialize();
    void setViewportSize(const glm::ivec2 &size);
    void render(const Camera &camera, const RenderData &data);
    void cleanup();

    PostProcessor &getPostProcessor();

private:
    void drawScene(const Camera &camera, const RenderData &data);
    ShadingProgram *getProgram(ShadingModel shading);

    std::map<ShadingModel, std::unique_ptr<ShadingProgram>> m_programs;
    PostProcessor m_postProcessor;
    glm::ivec2 m_viewportSize = glm::ivec2(0);
};
