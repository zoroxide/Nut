#include "Renderer.h"
#include "Terrain.h"
#include "Skybox.h"
#include "Models.h"

void Renderer::drawFrame(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& model,
                         const glm::mat4& invView, const glm::mat4& invProj,
                         const glm::vec3& cameraPos,
                         bool hasSkybox, float time,
                         bool cloudEnabled, float cloudSpeed, float cloudScale, float cloudOpacity) {
    if (sky_) sky_->setShader(skyProg_);
    if (sky_) sky_->draw(invView, invProj, hasSkybox, time, cloudEnabled, cloudSpeed, cloudScale, cloudOpacity);
    if (terrain_) terrain_->draw(terrainProg_, model, view, proj, cameraPos);
    if (models_) models_->drawAll(terrainProg_, view, proj);
}
