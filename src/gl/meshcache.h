#pragma once

#include "gl/mesh.h"
#include "interaction/aabb.h"

#include <map>
#include <string>

class MeshCache {
public:
    const Mesh &getMesh(const std::string &filepath);
    const AABB &getAABB(const std::string &filepath);
    const std::string &getMaterialName(const std::string &filepath);
    void clear();

private:
    struct Entry {
        Mesh mesh;
        AABB aabb;
        std::string material;
    };

    Entry &load(const std::string &filepath);

    std::map<std::string, Entry> m_entries;
};
