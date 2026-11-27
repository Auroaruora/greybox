#include "motions.h"

#include "debug/debug.h"
#include "gl/meshcache.h"
#include "screen/scene.h"
#include "utils/motionreader.h"

#include <glm/gtx/transform.hpp>

#include <cmath>
#include <limits>

static const float TwoPi = 6.28318530718f;

static glm::vec3 getPivot(const std::string &name, const glm::vec3 &min, const glm::vec3 &max) {
    glm::vec3 center = (min + max) * 0.5f;
    if (name == "base") {
        return glm::vec3(center.x, min.y, center.z);
    }
    if (name == "top") {
        return glm::vec3(center.x, max.y, center.z);
    }
    if (name == "origin") {
        return glm::vec3(0.0f);
    }
    return center;
}

static glm::mat3 getRotation(const glm::vec3 &degrees) {
    glm::mat4 matrix = glm::rotate(glm::radians(degrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    matrix = matrix * glm::rotate(glm::radians(degrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    return glm::mat3(matrix * glm::rotate(glm::radians(degrees.z), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void Motions::load(const Scene &scene, MeshCache &meshCache, const std::string &file) {
    m_motions.clear();
    m_time = 0.0f;
    std::vector<MotionSource> sources;
    if (!MotionReader::loadMotions(file, sources)) {
        return;
    }

    const std::vector<RenderShapeData> &shapes = scene.getShapes();
    for (const MotionSource &source : sources) {
        std::string name = source.part.empty() ? source.object + "_" : source.object + "_" + source.part;
        Motion motion;
        glm::vec3 min(std::numeric_limits<float>::max());
        glm::vec3 max(std::numeric_limits<float>::lowest());
        glm::mat4 holder(1.0f);
        for (int i = 0; i < static_cast<int>(shapes.size()); i++) {
            const std::string &shape = shapes[i].primitive.name;
            bool isMatch = source.part.empty() ? shape.starts_with(name) : shape == name;
            if (!isMatch) {
                continue;
            }
            const AABB &aabb = meshCache.getAABB(shapes[i].primitive.meshfile);
            min = glm::min(min, aabb.getMin());
            max = glm::max(max, aabb.getMax());
            holder = shapes[i].ctm;
            motion.shapes.push_back(i);
            motion.bases.push_back(shapes[i].ctm);
        }
        if (motion.shapes.empty()) {
            LOG_ERROR(file << ": nothing named " << name);
            continue;
        }
        motion.pivot = glm::vec3(holder * glm::vec4(getPivot(source.pivot, min, max), 1.0f));
        motion.frame = glm::mat3(glm::normalize(glm::vec3(holder[0])), glm::normalize(glm::vec3(holder[1])),
                                 glm::normalize(glm::vec3(holder[2])));
        motion.spin = source.spin;
        motion.sway = source.sway;
        motion.period = source.period;
        m_motions.push_back(motion);
    }
}

void Motions::update(Scene &scene, float deltaTime) {
    if (m_motions.empty()) {
        return;
    }
    m_time += deltaTime;
    for (const Motion &motion : m_motions) {
        glm::vec3 angles = motion.spin * m_time + motion.sway * std::sin(TwoPi * m_time / motion.period);
        glm::mat3 rotation = motion.frame * getRotation(angles) * glm::transpose(motion.frame);
        glm::mat4 transform = glm::translate(motion.pivot) * glm::mat4(rotation) * glm::translate(-motion.pivot);
        for (size_t i = 0; i < motion.shapes.size(); i++) {
            scene.setShapeTransform(motion.shapes[i], transform * motion.bases[i]);
        }
    }
}
