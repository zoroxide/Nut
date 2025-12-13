#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>

class Terrain;
class Skybox;
class Models;

class Renderer {
public:
    Renderer() = default;
    void setPrograms(GLuint terrainProg, GLuint skyProg) { terrainProg_ = terrainProg; skyProg_ = skyProg; }
    void setScene(Terrain* terrain, Skybox* sky, Models* models) { terrain_ = terrain; sky_ = sky; models_ = models; }

    void drawFrame(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& model,
                   const glm::mat4& invView, const glm::mat4& invProj,
                   const glm::vec3& cameraPos,
                   bool hasSkybox, float time,
                   bool cloudEnabled, float cloudSpeed, float cloudScale, float cloudOpacity);

private:
    GLuint terrainProg_ = 0;
    GLuint skyProg_ = 0;
    Terrain* terrain_ = nullptr;
    Skybox* sky_ = nullptr;
    Models* models_ = nullptr;
};
