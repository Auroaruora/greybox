#pragma once

#include "camera/camerapose.h"

#include <string>
#include <vector>

struct Viewpoint {
    std::string name;
    CameraPose pose;
};

class ViewpointReader {
public:
    ViewpointReader(const std::string &filename);

    bool readJSON();

    const std::vector<Viewpoint> &getViewpoints() const;

private:
    std::string m_filename;
    std::vector<Viewpoint> m_viewpoints;
};
