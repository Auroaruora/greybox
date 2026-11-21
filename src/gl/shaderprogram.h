#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>

class ShaderProgram {
public:
    ShaderProgram() = default;
    ShaderProgram(const ShaderProgram &) = delete;
    ShaderProgram &operator=(const ShaderProgram &) = delete;

    bool create(const std::string &vertexSource, const std::string &fragmentSource);
    void bind() const;
    void unbind() const;
    void cleanup();

    bool isValid() const;
    GLint getLocation(const std::string &name) const;
    void setSourceFiles(const std::string &vertexFile, const std::string &fragmentFile);

    void setInt(GLint location, int value) const;
    void setFloat(GLint location, float value) const;
    void setVec2(GLint location, const glm::vec2 &value) const;
    void setVec3(GLint location, const glm::vec3 &value) const;
    void setVec4(GLint location, const glm::vec4 &value) const;
    void setMat3(GLint location, const glm::mat3 &value) const;
    void setMat4(GLint location, const glm::mat4 &value) const;

private:
    GLuint m_id = 0;
    std::string m_vertexFile;
    std::string m_fragmentFile;
};
