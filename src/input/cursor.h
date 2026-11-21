#pragma once

#include "gl/texture2d.h"

#include <glm/glm.hpp>

#include <map>
#include <string>
#include <vector>

class SpriteRenderer;

class Cursor {
public:
    void initialize();
    void cleanup();

    void setIcon(const std::string &name);
    void update(const glm::ivec2 &windowSize);

    const std::string &getIcon() const;
    std::vector<std::string> getIconNames() const;
    glm::vec2 getPosition() const;
    glm::vec2 getNdc() const;

    void draw(const SpriteRenderer &sprites, const glm::ivec2 &windowSize) const;

private:
    std::map<std::string, Texture2D> m_icons;
    std::string m_icon;
    const Texture2D *m_texture = nullptr;
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_ndc = glm::vec2(0.0f);
};
