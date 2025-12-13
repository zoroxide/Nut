#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

class Skybox {
public:
    Skybox() = default;
    ~Skybox();

    bool setCubemap(GLuint texID);
    void setShader(GLuint prog);
    void initFullscreenTriangle();
    void draw(const glm::mat4& invView, const glm::mat4& invProj, bool hasSkybox, float time, bool cloudEnabled, float cloudSpeed, float cloudScale, float cloudOpacity);

    // High-level loading: load cubemap from directory (6 faces) or a single equirectangular image
    // Supports PNG and BMP. Returns true on success.
    bool loadFromPath(const std::string& path);
    bool hasCubemap() const { return cubemap_ != 0; }

private:
    GLuint skyShader_ = 0;
    GLuint skyVAO_ = 0;
    GLuint skyVBO_ = 0;
    GLuint cubemap_ = 0;
};
