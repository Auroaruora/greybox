#include "cursor.h"

#include "input/input.h"
#include "render/spriterenderer.h"
#include "utils/imageloader.h"
#include "utils/paths.h"

#include <filesystem>

static const float CursorPoints = 32.0f;
static const std::string DefaultIcon = "default";
static const std::string IconFolder = "ui/cursors";

void Cursor::initialize() {
    std::filesystem::path folder(Paths::getAssetPath(IconFolder));
    if (!std::filesystem::exists(folder)) {
        return;
    }
    for (const auto &entry : std::filesystem::directory_iterator(folder)) {
        if (entry.path().extension() != ".png") {
            continue;
        }
        Image image;
        if (ImageLoader::load(IconFolder + "/" + entry.path().filename().string(), image)) {
            m_icons[entry.path().stem().string()].create(image.width, image.height, image.pixels.data());
        }
    }
    setIcon(DefaultIcon);
}

void Cursor::cleanup() {
    for (auto &[name, texture] : m_icons) {
        texture.cleanup();
    }
    m_icons.clear();
    m_icon.clear();
    m_texture = nullptr;
}

void Cursor::setIcon(const std::string &name) {
    if (name == m_icon) {
        return;
    }
    auto found = m_icons.find(name);
    if (found != m_icons.end()) {
        m_icon = name;
        m_texture = &found->second;
    }
}

void Cursor::update(const glm::ivec2 &windowSize) {
    glm::vec2 bounds(windowSize);
    m_position = Input::getMousePosition();
    if (bounds.x > 0.0f && bounds.y > 0.0f) {
        m_ndc = glm::vec2(m_position.x / bounds.x * 2.0f - 1.0f, 1.0f - m_position.y / bounds.y * 2.0f);
    }
}

glm::vec2 Cursor::getPosition() const {
    return m_position;
}

glm::vec2 Cursor::getNdc() const {
    return m_ndc;
}

void Cursor::draw(const SpriteRenderer &sprites, const glm::ivec2 &windowSize) const {
    if (m_texture == nullptr) {
        return;
    }
    sprites.draw(*m_texture, m_position, glm::vec2(CursorPoints), glm::vec2(windowSize));
}
