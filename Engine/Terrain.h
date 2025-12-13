#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <string>
#include <vector>

// Simple Terrain facade to encapsulate procedural and flat terrain
class Terrain {
public:
    Terrain() = default;
    ~Terrain();

    // Build a procedural mesh using provided buffers (interleaved pos/normal/uv)
    // Ownership of VAO/VBO/EBO stays in Terrain.
    void buildProcedural(const std::vector<float>& interleaved, const std::vector<unsigned int>& indices);

    // Build a flat quad terrain and load its texture
    bool buildFlat(const std::string& texturePath);

    // Load a 2D texture to use for procedural terrain
    bool loadProceduralTexture(const std::string& texturePath);

    // High-level: generate a procedural terrain mesh using internal noise
    void generateProcedural(int size, float scale, float heightScale, float textureTile);

    // Query height at world x,z based on current procedural params
    float getHeightAt(float wx, float wz) const;

    // Draw currently configured terrain using given shader and matrices
    void draw(GLuint shaderProgram, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos);

    // Configure tiling and scale for flat draw
    void setFlatScale(const glm::vec3& s) { flatScale_ = s; }
         void setTextureTile(float t) { tile_ = t; }
         float getTextureTile() const { return tile_; }

    // State
    bool isFlat() const { return isFlat_; }

private:
    // Procedural
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    unsigned int indexCount_ = 0;
    GLuint procTexture_ = 0; // optional

    // Flat
    GLuint flatVAO_ = 0, flatVBO_ = 0, flatEBO_ = 0;
    GLuint flatTex_ = 0;
    glm::vec3 flatScale_{1.0f,1.0f,1.0f};
    bool isFlat_ = false;

    // Config for procedural generation
    int size_ = 512;
    float scale_ = 1.0f;
    float heightScale_ = 6.0f;
    float tile_ = 22.0f;
};
