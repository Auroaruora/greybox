#include "debug.h"

#include <cstring>
#include <vector>

namespace {
const std::vector<std::string> debugFiles = {
    "scene",
};
}

namespace debug {

bool isEnabled(const char *file) {
    std::string name = shortName(file);
    for (const std::string &pattern : debugFiles) {
        if (pattern == "all" || name.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

const char *shortName(const char *file) {
    const char *src = std::strstr(file, "/src/");
    return src ? src + 5 : file;
}

void print(std::ostream &out, const char *tag, const char *file, int line, const std::string &message) {
    std::string prefix = std::string("[") + tag + "] " + shortName(file) + ":" + std::to_string(line) + " ";
    std::string indent(prefix.size(), ' ');

    out << prefix << "| ";
    for (char c : message) {
        out << c;
        if (c == '\n') {
            out << indent << "| ";
        }
    }
    out << std::endl;
}

}
