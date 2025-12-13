#include "Models.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>

Models::~Models() {
    for (auto& inst : instances_) {
        if (inst.mesh.vbo) glDeleteBuffers(1, &inst.mesh.vbo);
        if (inst.mesh.ebo) glDeleteBuffers(1, &inst.mesh.ebo);
        if (inst.mesh.vao) glDeleteVertexArrays(1, &inst.mesh.vao);
    }
}

bool Models::loadOBJ(const std::string& path, glm::vec3 position, glm::vec3 scale) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        return false;
    }
    aiMesh* mesh = scene->mMeshes[0];
    std::vector<float> vertices; vertices.reserve(mesh->mNumVertices * 3);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
    }
    Mesh m; glGenVertexArrays(1, &m.vao); glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    m.indexCount = mesh->mNumVertices;
    instances_.push_back({m, position, scale});
    return true;
}

void Models::drawAll(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& proj) {
    for (const auto& inst : instances_) {
        glm::mat4 M = glm::translate(glm::mat4(1.0f), inst.pos) * glm::scale(glm::mat4(1.0f), inst.scale);
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, &(proj * view * M)[0][0]);
        glBindVertexArray(inst.mesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, inst.mesh.indexCount);
        glBindVertexArray(0);
    }
}
