#include "paths.h"

#include <filesystem>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

static std::filesystem::path findExecutableFolder() {
#ifdef __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        return std::filesystem::weakly_canonical(buffer.data()).parent_path();
    }
#endif
    return {};
}

static std::string findAssetsFolder() {
    std::filesystem::path folder = findExecutableFolder();
    if (!folder.empty()) {
        for (const std::filesystem::path &candidate : {folder / "assets", folder.parent_path() / "Resources" / "assets"}) {
            if (std::filesystem::is_directory(candidate)) {
                return candidate.string();
            }
        }
    }
    return ASSETS_DIR;
}

namespace Paths {

std::string getAssetPath(const std::string &relativePath) {
    static const std::string assetsFolder = findAssetsFolder();
    return assetsFolder + "/" + relativePath;
}

std::string getCapturePath(const std::string &filename) {
    return std::string(CAPTURES_DIR) + "/" + filename;
}

}