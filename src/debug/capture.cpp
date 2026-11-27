#include "capture.h"

#include "debug/debug.h"
#include "debug/screenshot.h"
#include "screen/screen.h"
#include "utils/paths.h"

#include <ctime>

static const std::string DebugCamera = "debug";

static std::string s_pendingFile;
static std::string s_lastPath;
static const Screen *s_batchScreen = nullptr;
static int s_batchStep = 0;
static std::string s_batchLighting;
static CameraPose s_batchCamera;

static std::string getFileName(const Screen &screen, int viewpoint) {
    std::string name = viewpoint >= 0 ? screen.getScene().getViewpoints()[viewpoint].name : "free";

    const std::string &lighting = screen.getScene().getLightingName();
    if (!lighting.empty()) {
        name += "_" + lighting;
    }

    const std::vector<SceneLightData> &lights = screen.getScene().getLights();
    if (!lights.empty()) {
        name += "_";
        for (const SceneLightData &light : lights) {
            name += light.enabled ? "1" : "0";
        }
    }

    if (viewpoint < 0) {
        char time[16];
        std::time_t now = std::time(nullptr);
        std::strftime(time, sizeof(time), "_%H%M%S", std::localtime(&now));
        name += time;
    }

    return screen.getName() + "/" + name + ".png";
}

static void finishBatch(Screen &screen) {
    if (!s_batchLighting.empty()) {
        screen.getScene().setLighting(s_batchLighting);
    }
    screen.getCameraSystem().setCamera(s_batchCamera);
    s_batchScreen = nullptr;
}

namespace Capture {

void request(const Screen &screen, int viewpoint) {
    s_pendingFile = getFileName(screen, viewpoint);
}

void startBatch(Screen &screen) {
    s_batchCamera = screen.getCameraSystem().getCamera().getPose();
    s_batchLighting = screen.getScene().getLightingName();
    s_batchStep = 0;
    s_batchScreen = &screen;
    screen.getCameraSystem().setController(DebugCamera);
}

void update(Screen &screen) {
    if (s_batchScreen == nullptr) {
        return;
    }
    if (s_batchScreen != &screen) {
        s_batchScreen = nullptr;
        return;
    }
    if (s_batchStep == getBatchTotal(screen)) {
        finishBatch(screen);
        return;
    }

    const std::vector<Viewpoint> &viewpoints = screen.getScene().getViewpoints();
    const std::vector<LightingState> &states = screen.getScene().getLightingStates();
    int viewpointCount = static_cast<int>(viewpoints.size());
    int state = s_batchStep / viewpointCount;
    int index = s_batchStep % viewpointCount;

    if (index == 0 && !states.empty()) {
        screen.getScene().setLighting(states[state].name);
    }
    screen.getCameraSystem().setCamera(viewpoints[index].pose);
    request(screen, index);
    s_batchStep++;
}

void save() {
    if (s_pendingFile.empty()) {
        return;
    }
    std::string path = Paths::getCapturePath(s_pendingFile);
    if (Screenshot::save(path)) {
        LOG_DEBUG("saved " << path);
        s_lastPath = path;
    } else {
        LOG_ERROR("cannot write " << path);
        s_lastPath.clear();
    }
    s_pendingFile.clear();
}

bool isBatchActive() {
    return s_batchScreen != nullptr;
}

int getBatchStep() {
    return s_batchStep;
}

int getBatchTotal(const Screen &screen) {
    int states = static_cast<int>(screen.getScene().getLightingStates().size());
    int viewpoints = static_cast<int>(screen.getScene().getViewpoints().size());
    return (states > 0 ? states : 1) * viewpoints;
}

const std::string &getLastPath() {
    return s_lastPath;
}

}
