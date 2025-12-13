#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    unsigned int indexCount = 0;
};

class Models {
public:
    Models() = default;
    ~Models();

    // Load a simple OBJ via Assimp and create a position-only mesh (for now)
    bool loadOBJ(const std::string& path, glm::vec3 position, glm::vec3 scale);
    void drawAll(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& proj);

private:
    struct Instance { Mesh mesh; glm::vec3 pos; glm::vec3 scale; };
    std::vector<Instance> instances_;
};
