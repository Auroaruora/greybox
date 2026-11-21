#pragma once

#include "gl/mesh.h"
#include "interaction/aabb.h"

#include <map>
#include <string>

class MeshCache {
public:
    const Mesh &getMesh(const std::string &filepath);
    const AABB &getAABB(const std::string &filepath);
    void clear();

private:
    struct Entry {
        Mesh mesh;
        AABB aabb;
    };

    Entry &load(const std::string &filepath);

    std::map<std::string, Entry> m_entries;
};
