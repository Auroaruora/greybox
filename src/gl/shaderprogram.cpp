#include "shaderprogram.h"

#include "debug/debug.h"

static std::string getShaderLog(GLuint shader) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(length, '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    return log;
}

static std::string getProgramLog(GLuint program) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(length, '\0');
    glGetProgramInfoLog(program, length, nullptr, log.data());
    return log;
}

static GLuint compileShader(GLenum type, const std::string &source) {
    GLuint shader = glCreateShader(type);
    const char *code = source.c_str();
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        LOG_ERROR((type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader failed to compile\n" << getShaderLog(shader));
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool ShaderProgram::create(const std::string &vertexSource, const std::string &fragmentSource) {
    cleanup();
    GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertex == 0 || fragment == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        LOG_ERROR("shader failed to link\n" << getProgramLog(program));
        glDeleteProgram(program);
        return false;
    }
    m_id = program;
    return true;
}

void ShaderProgram::bind() const {
    glUseProgram(m_id);
}

void ShaderProgram::unbind() const {
    glUseProgram(0);
}

void ShaderProgram::cleanup() {
    if (m_id != 0) {
        glDeleteProgram(m_id);
        m_id = 0;
    }
}

bool ShaderProgram::isValid() const {
    return m_id != 0;
}

GLint ShaderProgram::getLocation(const std::string &name) const {
    if (m_id == 0) {
        return -1;
    }
    GLint location = glGetUniformLocation(m_id, name.c_str());
    if (location == -1) {
        LOG_ERROR("uniform not found: " << name << " in " << m_vertexFile << " + " << m_fragmentFile);
    }
    return location;
}

void ShaderProgram::setSourceFiles(const std::string &vertexFile, const std::string &fragmentFile) {
    m_vertexFile = vertexFile;
    m_fragmentFile = fragmentFile;
}

void ShaderProgram::setInt(GLint location, int value) const {
    glUniform1i(location, value);
}

void ShaderProgram::setFloat(GLint location, float value) const {
    glUniform1f(location, value);
}

void ShaderProgram::setVec2(GLint location, const glm::vec2 &value) const {
    glUniform2fv(location, 1, &value[0]);
}

void ShaderProgram::setVec3(GLint location, const glm::vec3 &value) const {
    glUniform3fv(location, 1, &value[0]);
}

void ShaderProgram::setVec4(GLint location, const glm::vec4 &value) const {
    glUniform4fv(location, 1, &value[0]);
}

void ShaderProgram::setMat3(GLint location, const glm::mat3 &value) const {
    glUniformMatrix3fv(location, 1, GL_FALSE, &value[0][0]);
}

void ShaderProgram::setMat4(GLint location, const glm::mat4 &value) const {
    glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
}
