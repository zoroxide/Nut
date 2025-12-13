#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>

class Coins {
public:
    Coins() = default;
    ~Coins();

    void initMesh();
    void spawnGrid(int count, float radius, float y);
    // Spawn coins randomly over a rectangular area, placing them on terrain height
    // bounds: [minX,maxX] x [minZ,maxZ]
    void spawnRandomOnTerrain(int count, float minX, float maxX, float minZ, float maxZ, float hover, class Terrain& terrain);
    void update(float dt, const glm::vec3& cameraPos);
    void draw(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& proj);

    int getCollectedCount() const { return collected_; }

private:
    struct Coin { glm::vec3 pos; float phase; bool collected; };
    std::vector<Coin> coins_;
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei indexCount_ = 0;
    float spin_ = 0.0f;
    int collected_ = 0;
};
