#pragma once
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <cstdint>

using Entity = uint32_t;

struct Transform {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f}; // Euler radians
    glm::vec3 scale{1.0f};
};

struct CameraComponent {
    float fovDeg{60.0f};
    float nearZ{0.1f};
    float farZ{500.0f};
    bool active{true};
};

struct RenderMesh {
    unsigned int vao{0};
    unsigned int ebo{0};
    unsigned int vbo{0};
    unsigned int indexCount{0};
    unsigned int texture{0};
    unsigned int shader{0};
};

class World {
public:
    World() : next_(1) {}
    Entity create() { return next_++; }
    void destroy(Entity e);

    // Component maps
    std::unordered_map<Entity, Transform> transforms;
    std::unordered_map<Entity, CameraComponent> cameras;
    std::unordered_map<Entity, RenderMesh> meshes;

private:
    Entity next_;
};
