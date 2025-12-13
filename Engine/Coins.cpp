#include "Coins.h"
#include "Terrain.h"
#include <random>
#include <glm/gtc/matrix_transform.hpp>

Coins::~Coins() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
}

void Coins::initMesh() {
    if (vao_) return;
    struct V { glm::vec3 p; glm::vec3 n; glm::vec2 uv; };
    std::vector<V> verts = {
        {{0.5f,-0.5f,-0.5f},{1,0,0},{0,0}}, {{0.5f,0.5f,-0.5f},{1,0,0},{1,0}}, {{0.5f,0.5f,0.5f},{1,0,0},{1,1}}, {{0.5f,-0.5f,0.5f},{1,0,0},{0,1}},
        {{-0.5f,-0.5f,0.5f},{-1,0,0},{0,0}}, {{-0.5f,0.5f,0.5f},{-1,0,0},{1,0}}, {{-0.5f,0.5f,-0.5f},{-1,0,0},{1,1}}, {{-0.5f,-0.5f,-0.5f},{-1,0,0},{0,1}},
        {{-0.5f,0.5f,-0.5f},{0,1,0},{0,0}}, {{0.5f,0.5f,-0.5f},{0,1,0},{1,0}}, {{0.5f,0.5f,0.5f},{0,1,0},{1,1}}, {{-0.5f,0.5f,0.5f},{0,1,0},{0,1}},
        {{-0.5f,-0.5f,0.5f},{0,-1,0},{0,0}}, {{0.5f,-0.5f,0.5f},{0,-1,0},{1,0}}, {{0.5f,-0.5f,-0.5f},{0,-1,0},{1,1}}, {{-0.5f,-0.5f,-0.5f},{0,-1,0},{0,1}},
        {{-0.5f,-0.5f,0.5f},{0,0,1},{0,0}}, {{0.5f,-0.5f,0.5f},{0,0,1},{1,0}}, {{0.5f,0.5f,0.5f},{0,0,1},{1,1}}, {{-0.5f,0.5f,0.5f},{0,0,1},{0,1}},
        {{0.5f,-0.5f,-0.5f},{0,0,-1},{0,0}}, {{-0.5f,-0.5f,-0.5f},{0,0,-1},{1,0}}, {{-0.5f,0.5f,-0.5f},{0,0,-1},{1,1}}, {{0.5f,0.5f,-0.5f},{0,0,-1},{0,1}},
    };
    std::vector<unsigned int> idx;
    for (unsigned int f = 0; f < 6; ++f) {
        unsigned int base = f * 4;
        idx.push_back(base + 0); idx.push_back(base + 1); idx.push_back(base + 2);
        idx.push_back(base + 0); idx.push_back(base + 2); idx.push_back(base + 3);
    }
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(V), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    indexCount_ = (GLsizei)idx.size();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)offsetof(V, p));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)offsetof(V, n));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(V), (void*)offsetof(V, uv));
    glBindVertexArray(0);
}

void Coins::spawnGrid(int count, float radius, float y) {
    coins_.clear();
    for (int i = 0; i < count; ++i) {
        float ang = (float)i / (float)count * 6.2831853f;
        glm::vec3 p = glm::vec3(cos(ang) * radius, y, sin(ang) * radius);
        coins_.push_back({p, ang * 0.5f, false});
    }
}

// Random spawn across terrain bounds, hovering slightly above terrain
void Coins::spawnRandomOnTerrain(int count, float minX, float maxX, float minZ, float maxZ, float hover, class Terrain& terrain) {
    coins_.clear();
    if (count <= 0) return;
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> distX(minX, maxX);
    std::uniform_real_distribution<float> distZ(minZ, maxZ);
    std::uniform_real_distribution<float> distPhase(0.0f, 6.2831853f);
    for (int i = 0; i < count; ++i) {
        float x = distX(rng);
        float z = distZ(rng);
        float y = terrain.getHeightAt(x, z) + hover;
        coins_.push_back({glm::vec3(x, y, z), distPhase(rng), false});
    }
}

void Coins::update(float dt, const glm::vec3& cameraPos) {
    spin_ += dt * 1.5f;
    for (auto& c : coins_) {
        if (c.collected) continue;
        c.pos.y = c.pos.y + 0.25f * sinf(spin_ + c.phase) * dt;
        float dist = glm::length(glm::vec2(c.pos.x - cameraPos.x, c.pos.z - cameraPos.z));
        if (dist < 1.2f) { c.collected = true; collected_++; }
    }
}

void Coins::draw(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& proj) {
    if (!vao_) return;
    glUseProgram(shaderProgram);
    glBindVertexArray(vao_);
    // Set solid gold color for coins
    GLint locUseSolid = glGetUniformLocation(shaderProgram, "useSolidColor");
    GLint locSolidCol = glGetUniformLocation(shaderProgram, "solidColor");
    if (locUseSolid >= 0) glUniform1i(locUseSolid, 1);
    if (locSolidCol >= 0) glUniform3f(locSolidCol, 1.0f, 0.84f, 0.0f);
    for (const auto& c : coins_) {
        if (c.collected) continue;
        glm::mat4 M(1.0f);
        M = glm::translate(M, c.pos);
        M = glm::rotate(M, spin_, glm::vec3(0,1,0));
        M = glm::scale(M, glm::vec3(0.2f, 0.2f, 0.05f));
        glm::mat4 MVP = proj * view * M;
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, &MVP[0][0]);
        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
    }
    // restore optional flag
    if (locUseSolid >= 0) glUniform1i(locUseSolid, 0);
    glBindVertexArray(0);
}
