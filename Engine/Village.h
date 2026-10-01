#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
class Terrain;

// Static village geometry is combined into one material-array mesh.
class Village {
public:
    void generate(Terrain& terrain);
    void clear();
    void draw(GLuint program, const glm::mat4& view, const glm::mat4& proj) const;
    // Local depth shadows supplement the island-wide terrain height shadows.
    void buildShadow(GLuint program, const glm::vec3& sunDir);
    void bindShadow(GLuint program, int unit, float strength, bool enabled) const;
    void collide(glm::vec3& eye) const;
    bool active() const { return count_ != 0; }
    glm::vec3 center() const { return origin_; }
    glm::vec3 spawn() const { return origin_ + glm::vec3(0, 1.7f, 20); }
    glm::vec3 clearing() const { return active() ? glm::vec3(origin_.x, origin_.z, 49) : glm::vec3(0); }
private:
    struct Vertex { glm::vec3 p, n; glm::vec2 uv; float material; glm::vec3 tint; };
    struct Wall { glm::vec3 lo, hi; };
    std::vector<Vertex> vertices_;
    std::vector<Wall> walls_;
    glm::vec3 origin_{0};
    GLuint vao_ = 0, vbo_ = 0, albedo_ = 0, normal_ = 0;
    GLuint shadowFbo_ = 0, shadowTex_ = 0;
    glm::mat4 shadowMatrix_{1.0f};
    glm::vec3 shadowSun_{0.0f};
    bool shadowBuilt_ = false;
    static constexpr int kShadowResolution = 2048;
    GLsizei count_ = 0;
    void quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int mat, glm::vec3 tint);
    void box(glm::vec3 p, glm::vec3 size, int mat, bool solid = true, glm::vec3 tint = glm::vec3(1));
    void house(float x, float z, bool north, int variant);
};
