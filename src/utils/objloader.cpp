#include "objloader.h"

#include "debug/debug.h"
#include "utils/paths.h"

#include <glm/glm.hpp>

#include <fstream>
#include <sstream>

namespace {

struct VertexIndex {
    int position = -1;
    int uv = -1;
    int normal = -1;
};

struct ObjData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
};

int toIndex(const std::string &text, size_t count) {
    if (text.empty()) {
        return -1;
    }
    int index = std::stoi(text);
    return index > 0 ? index - 1 : static_cast<int>(count) + index;
}

bool inRange(int index, size_t count) {
    return index >= 0 && index < static_cast<int>(count);
}

VertexIndex parseVertexIndex(const std::string &token, const ObjData &obj) {
    std::istringstream stream(token);
    std::string position, uv, normal;
    std::getline(stream, position, '/');
    std::getline(stream, uv, '/');
    std::getline(stream, normal, '/');

    VertexIndex index;
    index.position = toIndex(position, obj.positions.size());
    index.uv = toIndex(uv, obj.uvs.size());
    index.normal = toIndex(normal, obj.normals.size());
    return index;
}

void appendVertex(std::vector<float> &vertexData, glm::vec3 position, glm::vec3 normal, glm::vec2 uv) {
    vertexData.insert(vertexData.end(), {position.x, position.y, position.z,
                                         normal.x, normal.y, normal.z,
                                         uv.x, uv.y});
}

bool appendTriangle(std::vector<float> &vertexData, const ObjData &obj, const VertexIndex (&triangle)[3]) {
    for (const VertexIndex &index : triangle) {
        if (!inRange(index.position, obj.positions.size())) {
            return false;
        }
    }

    glm::vec3 a = obj.positions[triangle[0].position];
    glm::vec3 b = obj.positions[triangle[1].position];
    glm::vec3 c = obj.positions[triangle[2].position];
    glm::vec3 faceNormal = glm::cross(b - a, c - a);
    faceNormal = glm::length(faceNormal) > 0.0f ? glm::normalize(faceNormal) : glm::vec3(0.0f, 1.0f, 0.0f);

    for (const VertexIndex &index : triangle) {
        glm::vec3 position = obj.positions[index.position];
        glm::vec3 normal = inRange(index.normal, obj.normals.size()) ? obj.normals[index.normal] : faceNormal;
        glm::vec2 uv = inRange(index.uv, obj.uvs.size()) ? obj.uvs[index.uv] : glm::vec2(0.0f);
        appendVertex(vertexData, position, normal, uv);
    }
    return true;
}

}

bool ObjLoader::load(const std::string &filepath, std::vector<float> &vertexData, std::string &material) {
    std::ifstream file(Paths::getAssetPath(filepath));
    if (!file.is_open()) {
        LOG_ERROR("cannot open " << filepath);
        return false;
    }

    ObjData obj;
    vertexData.clear();
    material.clear();
    int skipped = 0;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string type;
        stream >> type;

        if (type == "usemtl" && material.empty()) {
            stream >> material;
        } else if (type == "v") {
            glm::vec3 position(0.0f);
            stream >> position.x >> position.y >> position.z;
            obj.positions.push_back(position);
        } else if (type == "vt") {
            glm::vec2 uv(0.0f);
            stream >> uv.x >> uv.y;
            obj.uvs.push_back(uv);
        } else if (type == "vn") {
            glm::vec3 normal(0.0f);
            stream >> normal.x >> normal.y >> normal.z;
            obj.normals.push_back(normal);
        } else if (type == "f") {
            std::vector<VertexIndex> face;
            std::string token;
            while (stream >> token && token[0] != '#') {
                face.push_back(parseVertexIndex(token, obj));
            }
            for (size_t i = 1; i + 1 < face.size(); i++) {
                if (!appendTriangle(vertexData, obj, {face[0], face[i], face[i + 1]})) {
                    skipped++;
                }
            }
        }
    }

    if (skipped > 0) {
        LOG_WARN(filepath << ": skipped " << skipped << " triangles with invalid indices");
    }
    LOG_DEBUG("loaded " << filepath << ": " << vertexData.size() / 24 << " triangles");
    return true;
}