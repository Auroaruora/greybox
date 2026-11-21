#include "meshcache.h"

#include "utils/objloader.h"

#include <vector>

MeshCache::Entry &MeshCache::load(const std::string &filepath) {
    auto it = m_entries.find(filepath);
    if (it != m_entries.end()) {
        return it->second;
    }

    Entry &entry = m_entries[filepath];
    std::vector<float> vertexData;
    if (ObjLoader::load(filepath, vertexData)) {
        entry.mesh.create(vertexData);
        for (size_t v = 0; v + 2 < vertexData.size(); v += Mesh::FloatsPerVertex) {
            entry.aabb.expand(glm::vec3(vertexData[v], vertexData[v + 1], vertexData[v + 2]));
        }
    }
    return entry;
}

const Mesh &MeshCache::getMesh(const std::string &filepath) {
    return load(filepath).mesh;
}

const AABB &MeshCache::getAABB(const std::string &filepath) {
    return load(filepath).aabb;
}

void MeshCache::clear() {
    for (auto &[filepath, entry] : m_entries) {
        entry.mesh.cleanup();
    }
    m_entries.clear();
}
