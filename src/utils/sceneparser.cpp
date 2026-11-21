#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

static glm::mat4 getTransformMatrix(const SceneTransformation* t) {
    switch (t->type) {
    case TransformationType::TRANSFORMATION_TRANSLATE:
        return glm::translate(t->translate);
    case TransformationType::TRANSFORMATION_SCALE:
        return glm::scale(t->scale);
    case TransformationType::TRANSFORMATION_ROTATE:
        return glm::rotate(t->angle, t->rotate);
    case TransformationType::TRANSFORMATION_MATRIX:
        return t->matrix;
    }
    return glm::mat4(1.0f);
}

static void traverse(SceneNode* node, glm::mat4 ctm, RenderData& renderData) {
    for (SceneTransformation* t : node->transformations) {
        ctm = ctm * getTransformMatrix(t);
    }

    for (ScenePrimitive* prim : node->primitives) {
        RenderShapeData shape;
        shape.primitive  = *prim;
        shape.ctm        = ctm;
        shape.inverseCtm = glm::inverse(ctm);
        shape.normalMatrix = glm::transpose(glm::mat3(shape.inverseCtm));
        renderData.shapes.push_back(shape);
    }

    for (SceneLight* light : node->lights) {
        SceneLightData lightData;
        lightData.id       = light->id;
        lightData.type     = light->type;
        lightData.name     = light->name;
        lightData.emitter  = light->emitter;
        lightData.color    = light->color;
        lightData.function = light->function;
        lightData.penumbra = light->penumbra;
        lightData.angle    = light->angle;
        lightData.width    = light->width;
        lightData.height   = light->height;
        lightData.pos = ctm * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);        // w = 1: a point, so translation applies
        lightData.dir = ctm * glm::vec4(glm::vec3(light->dir), 0.0f);   // w = 0: a direction, so translation is ignored
        renderData.lights.push_back(lightData);
    }

    for (SceneNode* child : node->children) {
        traverse(child, ctm, renderData);
    }
}


bool SceneParser::parse(const std::string &filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();

    renderData.shapes.clear();
    renderData.lights.clear();

    traverse(fileReader.getRootNode(), glm::mat4(1.0f), renderData);

    return true;
}
