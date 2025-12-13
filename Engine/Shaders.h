#pragma once

#include <string>
#include <unordered_map>
#include <GL/glew.h>

class ShaderManager {
public:
    ShaderManager() = default;
    ~ShaderManager();

    GLuint loadProgram(const std::string& name, const char* vsPath, const char* fsPath);
    GLuint get(const std::string& name) const;

private:
    GLuint compileShaderFromFile(const char* path, GLenum type);
    GLuint createProgram(const char* vsPath, const char* fsPath);

    std::unordered_map<std::string, GLuint> programs_;
};
