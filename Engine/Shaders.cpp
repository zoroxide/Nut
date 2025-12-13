#include "Shaders.h"
#include <fstream>
#include <sstream>
#include <iostream>

static std::string loadFile(const char* path) {
    std::ifstream in(path);
    if(!in) { std::cerr << "Failed to open " << path << std::endl; return {}; }
    std::stringstream ss; ss << in.rdbuf(); return ss.str();
}

ShaderManager::~ShaderManager(){
    for (auto& kv : programs_) {
        if (kv.second) glDeleteProgram(kv.second);
    }
}

GLuint ShaderManager::compileShaderFromFile(const char* path, GLenum type) {
    std::string src = loadFile(path);
    if(src.empty()) return 0;
    const char* csrc = src.c_str();
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &csrc, nullptr);
    glCompileShader(sh);
    GLint ok; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if(!ok) { char buf[4096]; glGetShaderInfoLog(sh, 4096, nullptr, buf); std::cerr << "Shader compile error (" << path << ")\n" << buf << std::endl; }
    return sh;
}

GLuint ShaderManager::createProgram(const char* vsPath, const char* fsPath) {
    GLuint vs = compileShaderFromFile(vsPath, GL_VERTEX_SHADER);
    GLuint fs = compileShaderFromFile(fsPath, GL_FRAGMENT_SHADER);
    if(!vs || !fs) return 0;
    GLuint prog = glCreateProgram(); glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if(!ok) { char buf[4096]; glGetProgramInfoLog(prog, 4096, nullptr, buf); std::cerr << "Program link error:\n" << buf << std::endl; }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

GLuint ShaderManager::loadProgram(const std::string& name, const char* vsPath, const char* fsPath) {
    auto it = programs_.find(name);
    if (it != programs_.end() && it->second) return it->second;
    GLuint p = createProgram(vsPath, fsPath);
    if (p) programs_[name] = p;
    return p;
}

GLuint ShaderManager::get(const std::string& name) const {
    auto it = programs_.find(name);
    return (it != programs_.end()) ? it->second : 0;
}
