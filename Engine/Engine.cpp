#include "Engine.h"
#include "gui/gui.h"

#define GLM_ENABLE_EXPERIMENTAL

// GLMs
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

// STLs
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// Vertex struct
struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

// Static instance pointer
Engine *Engine::s_instance_ = nullptr;

Engine::Engine()
    : window_(nullptr), shaderProgram_(0), skyShader_(0),
      cameraPos_(0.0f, 6.0f, 12.0f), yaw_(-90.0f), pitch_(-15.0f),
      mouseSensitivity_(0.12f), moveSpeed_(6.0f), lastX_(0.0), lastY_(0.0),
      firstMouse_(true), lastFrame_(Clock::now()), deltaTime_(0.0f),
      jumping_(false), jumpVel_(0.0f), vsyncEnabled_(true) {
  std::fill(std::begin(keys_), std::end(keys_), false);
  s_instance_ = this;

  // Defaults for configurable constants and paths
  terrainSize_ = 1024;
  terrainScale_ = 1.2f;
  heightScale_ = 100.0f;
  textureTile_ = 22.0f;
  panoramaPath_.clear();
  terrainTexturePath_.clear();
  // Cloud defaults
  cloudEnabled_ = true;
  cloudSpeed_ = 0.02f;
  cloudScale_ = 1.0f;
  cloudOpacity_ = 0.55f;

  // Create GUI manager (will be initialized after window/context creation)
  gui_ = new GUI(this);
}

Engine::~Engine() {
  // Cleanup
  if (shaderProgram_)
    glDeleteProgram(shaderProgram_);
  if (skyShader_)
    glDeleteProgram(skyShader_);
  // Skybox VAO/VBO are managed by Skybox class
  if (window_)
    glfwTerminate();

  if (gui_) {
    delete gui_;
    gui_ = nullptr;
  }
}

bool Engine::init(bool fullscreen) {
  // glfw init
  if (!glfwInit())
    return false;

  // glfw window hints
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create window
  GLFWmonitor *monitor = nullptr;
  int SCR_W = 1280, SCR_H = 720;

  // Fullscreen setup
  if (fullscreen) {
    monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    SCR_W = mode->width;
    SCR_H = mode->height;
  }

  // Create window
  window_ = glfwCreateWindow(SCR_W, SCR_H, "Procedural Terrain (Engine)",
                             monitor, nullptr);
  if (!window_) {
    glfwTerminate();
    return false;
  }

  // GLEW + GL context
  glfwMakeContextCurrent(window_);
  glfwSwapInterval(vsyncEnabled_ ? 1 : 0);
  glewExperimental = GL_TRUE;
  GLenum glewErr = glewInit();
#ifdef GLEW_ERROR_NO_GLX_DISPLAY
  // On Wayland, GLEW (built for GLX) reports "no GLX display" but still loads
  // every GL function, so that error is harmless.
  if (glewErr == GLEW_ERROR_NO_GLX_DISPLAY)
    glewErr = GLEW_OK;
#endif
  if (glewErr != GLEW_OK) {
    std::cerr << "glewInit failed: " << glewGetErrorString(glewErr) << "\n";
    return false;
  }

  // Input
  glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window_, Engine::cursorPosCallbackStatic);
  glfwSetKeyCallback(window_, Engine::keyCallbackStatic);

  // GL settings
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // Resources Loading(shaders, terrain mesh, etc)
  shaderProgram_ = shaders_.loadProgram("terrain", "Engine/shaders/vertex.glsl",
                                        "Engine/shaders/fragment.glsl");
  // Reuse terrain shader initially for simple model rendering.
  // For robust rendering, a separate model shader can be added later.

  // Create sky shader and setup Skybox helper
  skyShader_ = shaders_.loadProgram("sky", "Engine/shaders/sky_vert.glsl",
                                    "Engine/shaders/sky_frag.glsl");
  sky_.setShader(skyShader_);
  waterShader_ = shaders_.loadProgram("water", "Engine/shaders/water_vert.glsl",
                                      "Engine/shaders/water_frag.glsl");
  terrain_.loadMaterials("assets/textures/terrain");
  sky_.initFullscreenTriangle();

  // Renderer programs and scene wiring
  renderer_.setPrograms(shaderProgram_, skyShader_);
  // Models are drawn by the engine loop (before the transparent ocean), not the renderer
  renderer_.setScene(&terrain_, &sky_, nullptr);

  // Generate initial procedural terrain via Terrain subsystem
  terrain_.generateProcedural(terrainSize_, terrainScale_, heightScale_,
                              textureTile_, terrainParams_);
  placePlayerOnLand();

  // Initialize GUI after the OpenGL context is created
  if (gui_)
    gui_->init(window_);

  return true;
}

void Engine::vsync(bool enabled) {
  vsyncEnabled_ = enabled;
  if (window_)
    glfwSwapInterval(enabled ? 1 : 0);
}

bool Engine::getVsyncEnabled() const { return vsyncEnabled_; }

/* (Getters / Setters) */

// Terrain accessors
int Engine::getTerrainSize() const { return terrainSize_; }
void Engine::setTerrainSize(int v) { terrainSize_ = v; }
float Engine::getTerrainScale() const { return terrainScale_; }
void Engine::setTerrainScale(float v) { terrainScale_ = v; }
float Engine::getHeightScale() const { return heightScale_; }
void Engine::setHeightScale(float v) { heightScale_ = v; }
float Engine::getTextureTile() const { return textureTile_; }
void Engine::setTextureTile(float v) { textureTile_ = v; }

// Path accessors
const std::string &Engine::getPanoramaPath() const { return panoramaPath_; }
void Engine::setPanoramaPath(const std::string &p) { panoramaPath_ = p; }
const std::string &Engine::getTerrainTexturePath() const {
  return terrainTexturePath_;
}
void Engine::setTerrainTexturePath(const std::string &p) {
  terrainTexturePath_ = p;
}

// Cloud accessors
bool Engine::getCloudEnabled() const { return cloudEnabled_; }
void Engine::setCloudEnabled(bool v) { cloudEnabled_ = v; }
float Engine::getCloudSpeed() const { return cloudSpeed_; }
void Engine::setCloudSpeed(float v) { cloudSpeed_ = v; }
float Engine::getCloudScale() const { return cloudScale_; }
void Engine::setCloudScale(float v) { cloudScale_ = v; }
float Engine::getCloudOpacity() const { return cloudOpacity_; }
void Engine::setCloudOpacity(float v) { cloudOpacity_ = v; }

void Engine::load_terrain_using_texture(const std::string &texturePath,
                                        const std::string &objPath) {
  // Load a procedural terrain texture via Terrain subsystem
  if (!texturePath.empty()) {
    terrain_.loadProceduralTexture(texturePath);
  }

  if (!objPath.empty()) {
    models_.loadOBJ(objPath, glm::vec3(0.0f), glm::vec3(1.0f));
  }
}

void Engine::add_house(const std::string &objPath, const glm::vec3 &position,
                       const glm::vec3 &scale) {
  // Delegate to Models manager
  if (!models_.loadOBJ(objPath, position, scale)) {
    std::cerr << "Error: Failed to load house OBJ via Models: " << objPath
              << "\n";
  }
}

bool Engine::load_flat_terrain(const std::string &texturePath) {
  // Propagate current texture tiling to Terrain so flat UVs are repeated
  terrain_.setTextureTile(textureTile_);
  hasFlat_ = terrain_.buildFlat(texturePath);
  if (!hasFlat_)
    return false;
  terrain_.setFlatScale(glm::vec3(terrainSize_ * terrainScale_ * 0.5f, 1.0f,
                                  terrainSize_ * terrainScale_ * 0.5f));

  // Also place a house model in front of the player when switching to flat
  // terrain. Use the current yaw to compute forward direction on XZ, and
  // position the house a few meters ahead.
  {
    // Place a properly sized house in front of the player on the flat plane
    glm::vec3 forward = glm::normalize(
        glm::vec3(cos(glm::radians(yaw_)), 0.0f, sin(glm::radians(yaw_))));
    glm::vec3 housePos =
        cameraPos_ +
        forward * 12.0f; // a bit farther ahead to avoid immediate collision
    housePos.y = 0.01f;  // slight lift to avoid z-fighting with the plane
    // Use a larger uniform scale so the user can enter the house comfortably
    glm::vec3 houseScale = glm::vec3(6.0f);
    // Load the house OBJ via Models subsystem (encapsulated loading/drawing)
    models_.loadOBJ("assets/objs/house.obj", housePos, houseScale);
  }
  return true;
}
void Engine::mainloop() {
  // Safety check
  if (!window_)
    return;

  // Sampler units that never change (terrain shader): 0 = surface texture,
  // 2 = height map (water), 3 = sky cubemap. Each sampler type needs its own unit.
  glUseProgram(shaderProgram_);
  glUniform1i(glGetUniformLocation(shaderProgram_, "texture1"), 0);
  glUniform1i(glGetUniformLocation(shaderProgram_, "heightTex"), 2);
  glUniform1i(glGetUniformLocation(shaderProgram_, "skyTex"), 3);
  glUseProgram(waterShader_);
  glUniform1i(glGetUniformLocation(waterShader_, "heightTex"), 2);
  glUniform1i(glGetUniformLocation(waterShader_, "skyTex"), 3);

  // Get initial window size
  int SCR_W, SCR_H;
  glfwGetWindowSize(window_, &SCR_W, &SCR_H);

  // Main loop
  lastFrame_ = Clock::now();
  while (!glfwWindowShouldClose(window_)) {
    // Timing
    auto now = Clock::now();
    deltaTime_ = std::chrono::duration<float>(now - lastFrame_).count();
    lastFrame_ = now;
    terrain_.setLiveParams(terrainParams_);
    updateMovement(deltaTime_);

    // Camera
    // Camera matrices
    camera_.setPosition(cameraPos_);
    camera_.setYawPitch(yaw_, pitch_);
    glm::mat4 view = camera_.getView();
    glm::mat4 proj =
        camera_.getProj(underwater_ ? 70.0f : 60.0f, (float)SCR_W / (float)SCR_H, 0.2f, 4000.0f);
    glm::mat4 model(1.0f);

    // --- Clear first (important!) ---
    glClearColor(0.53f, 0.8f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 invProj = glm::inverse(proj);
    glm::mat4 invView = glm::inverse(view);

    // Use running time since program start for smoother animation
    static double startTime =
        std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
    float runTime =
        (float)(std::chrono::duration<double>(Clock::now().time_since_epoch())
                    .count() -
                startTime);

    // Light the scene to match the sky (sun direction/colour detected from HDR panoramas)
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));
    glm::vec3 lightCol(1.0f, 0.98f, 0.9f);
    if (sunFromSky_) {
      lightDir = sky_.lightDirection();
      lightCol = sky_.lightColor();
    }
    // Colour of the water seen from inside it (dimmer when the sun is weak)
    const TerrainParams &TP = terrain_.params();
    float sunLum = glm::clamp(glm::dot(lightCol, glm::vec3(0.3f, 0.6f, 0.1f)), 0.25f, 1.0f);
    glm::vec3 uwColor = glm::mix(TP.waterDeep, TP.waterShallow, 0.45f) * (0.35f + 0.9f * sunLum);
    setPerFrameUniforms(shaderProgram_, lightDir, lightCol, uwColor);
    setPerFrameUniforms(waterShader_, lightDir, lightCol, uwColor);
    glUseProgram(skyShader_);
    glUniform1i(glGetUniformLocation(skyShader_, "underwater"), underwater_ ? 1 : 0);
    glUniform3fv(glGetUniformLocation(skyShader_, "uwColor"), 1, &uwColor.x);
    sky_.fogColor = terrainParams_.fogColor;

    // --- Then draw terrain via Renderer (which calls Terrain) ---
    renderer_.drawFrame(view, proj, model, invView, invProj, cameraPos_,
                        sky_.hasCubemap(), runTime, cloudEnabled_, cloudSpeed_,
                        cloudScale_, cloudOpacity_);


  // --- Draw models via Models manager ---
    models_.drawAll(shaderProgram_, view, proj);

    // --- Ocean last: it is transparent and must blend over everything below it ---
    terrain_.drawWater(waterShader_, view, proj, cameraPos_, swimTime_);

    // Render GUI
  gui_->render();

    // Swap buffers and poll events
    glfwSwapBuffers(window_);
    glfwPollEvents();
  }
}

// ---------------- Utility / helpers ----------------

bool Engine::panorama(const std::string &path) {
  if (!sky_.loadFromPath(path))
    return false;
  panoramaPath_ = path;
  // Photographic skies already contain clouds; the procedural layer is for the
  // built-in sky (it can still be re-enabled from the GUI)
  cloudEnabled_ = path.empty();
  return true;
}

// Input callbacks
void Engine::cursorPosCallbackStatic(GLFWwindow *, double xpos, double ypos) {
  if (s_instance_)
    s_instance_->cursorPosCallback(xpos, ypos);
}
void Engine::keyCallbackStatic(GLFWwindow *window, int key, int scancode,
                               int action, int mods) {
  if (s_instance_)
    s_instance_->keyCallback(key, scancode, action, mods);
}

void Engine::cursorPosCallback(double xpos, double ypos) {
  if (firstMouse_) {
    lastX_ = xpos;
    lastY_ = ypos;
    firstMouse_ = false;
  }
  double xoff = xpos - lastX_;
  double yoff = lastY_ - ypos;
  lastX_ = xpos;
  lastY_ = ypos;
  xoff *= mouseSensitivity_;
  yoff *= mouseSensitivity_;
  yaw_ += (float)xoff;
  pitch_ += (float)yoff;
  pitch_ = glm::clamp(pitch_, -89.0f, 89.0f);
}

void Engine::keyCallback(int key, int, int action, int) {
  if (key >= 0 && key < 1024)
    keys_[key] = (action == GLFW_PRESS || action == GLFW_REPEAT); // key states

  // ESC closes the window
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window_, true);

  // SPACE for jumping
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !jumping_ && !swimming_) {
    jumping_ = true;
    jumpVel_ = JUMP_VELOCITY; // ideal 7 for normal jump
  }

  // ENTER toggles mouse visibility
  if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
    static bool cursorVisible = false;
    cursorVisible = !cursorVisible;
    if (cursorVisible)
      glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
      glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }
}

void Engine::setPerFrameUniforms(GLuint prog, const glm::vec3 &lightDir,
                                 const glm::vec3 &lightCol,
                                 const glm::vec3 &uwColor) {
  glUseProgram(prog);
  glUniform3fv(glGetUniformLocation(prog, "lightDir"), 1, &lightDir.x);
  glUniform3fv(glGetUniformLocation(prog, "lightColor"), 1, &lightCol.x);
  glUniform1i(glGetUniformLocation(prog, "fogFromSky"), fogFromSky_ ? 1 : 0);
  glUniform1i(glGetUniformLocation(prog, "underwater"), underwater_ ? 1 : 0);
  glUniform3fv(glGetUniformLocation(prog, "uwColor"), 1, &uwColor.x);
  glUniform1f(glGetUniformLocation(prog, "time"), swimTime_);
  sky_.bindForLighting(prog, 3);
}

void Engine::updateMovement(float dt) {
  // Walking: WASD + SPACE to jump, SHIFT to sprint.
  // Swimming (deep water): the player floats at the surface and rides the waves.
  //   W swims where you look (look down to dive), SPACE swims up, CTRL / C dives,
  //   SHIFT swims faster. Oxygen drains while the head is under water.
  dt = std::min(dt, 0.1f); // avoid tunnelling after a stall
  swimTime_ += dt;

  const float eyeHeight = 1.7f;
  const float floatEye = 0.35f;   // eye height above the surface while floating
  const float swimDepth = 1.35f;  // water deeper than this and you start swimming

  glm::vec3 flatFront = glm::normalize(
      glm::vec3(cos(glm::radians(yaw_)), 0.0f, sin(glm::radians(yaw_))));
  glm::vec3 right = glm::normalize(glm::cross(flatFront, glm::vec3(0, 1, 0)));
  glm::vec3 lookFront = camera_.forward();

  auto input = [&](const glm::vec3 &fwd) {
    glm::vec3 m(0.0f);
    if (keys_[GLFW_KEY_W]) m += fwd;
    if (keys_[GLFW_KEY_S]) m -= fwd;
    if (keys_[GLFW_KEY_A]) m -= right;
    if (keys_[GLFW_KEY_D]) m += right;
    return glm::length(m) > 0.0f ? glm::normalize(m) : m;
  };

  float ground = terrain_.getHeightAt(cameraPos_.x, cameraPos_.z);
  float surface = terrain_.getWaterSurfaceAt(cameraPos_.x, cameraPos_.z, swimTime_);
  float depth = surface - ground;

  if (!swimming_) {
    // Wading slows you down as the water gets deeper
    float wade = glm::clamp(depth / swimDepth, 0.0f, 1.0f);
    float sp = moveSpeed_ * (keys_[GLFW_KEY_LEFT_SHIFT] ? SPRINT_MULTIPLIER : 1.0f) *
               (1.0f - 0.55f * wade);
    cameraPos_ += input(flatFront) * sp * dt;
    ground = terrain_.getHeightAt(cameraPos_.x, cameraPos_.z);
    surface = terrain_.getWaterSurfaceAt(cameraPos_.x, cameraPos_.z, swimTime_);
    depth = surface - ground;

    if (jumping_) {
      cameraPos_.y += jumpVel_ * dt;
      jumpVel_ -= 18.0f * dt;
      if (cameraPos_.y <= ground + eyeHeight) {
        cameraPos_.y = ground + eyeHeight;
        jumping_ = false;
        jumpVel_ = 0.0f;
      }
    } else {
      cameraPos_.y = ground + eyeHeight;
    }
    // Deep enough to float: start swimming (also when jumping/falling into the sea)
    if (depth > swimDepth && cameraPos_.y < surface + eyeHeight) {
      swimming_ = true;
      jumping_ = false;
      jumpVel_ = std::min(jumpVel_, 0.0f) * 0.3f; // splash: keep a little downward momentum
    }
  } else {
    bool fast = keys_[GLFW_KEY_LEFT_SHIFT];
    float sp = (fast ? 4.8f : 2.8f);
    bool headUnder = cameraPos_.y < surface - 0.15f;
    // At the surface W swims flat unless you look well down (to dive); under water it follows the view
    glm::vec3 fwd = (headUnder || lookFront.y < -0.45f) ? lookFront : flatFront;
    glm::vec3 move = input(fwd) * sp;

    // Vertical: SPACE up, CTRL/C down, otherwise buoyancy pulls you to the surface
    float vy = move.y;
    bool up = keys_[GLFW_KEY_SPACE] || oxygen_ <= 0.0f;
    bool down = (keys_[GLFW_KEY_LEFT_CONTROL] || keys_[GLFW_KEY_C]) && oxygen_ > 0.0f;
    if (up) vy += oxygen_ <= 0.0f ? 4.0f : 2.5f;
    if (down) vy -= 2.5f;
    float target = surface + floatEye;
    if (!up && !down && std::fabs(move.y) < 0.2f) {
      if (cameraPos_.y > target - 1.2f)
        vy += (target - cameraPos_.y) * 7.0f;   // settle on the surface and ride the waves
      else
        vy += 0.9f;                               // buoyancy from deeper down
    }
    // Leftover fall speed from a jump/dive into the water, damped by drag
    jumpVel_ *= std::exp(-3.0f * dt);
    vy += jumpVel_;

    cameraPos_ += glm::vec3(move.x, 0.0f, move.z) * dt;
    cameraPos_.y += vy * dt;

    ground = terrain_.getHeightAt(cameraPos_.x, cameraPos_.z);
    surface = terrain_.getWaterSurfaceAt(cameraPos_.x, cameraPos_.z, swimTime_);
    depth = surface - ground;
    cameraPos_.y = std::min(cameraPos_.y, surface + floatEye);  // can't fly out of the water
    cameraPos_.y = std::max(cameraPos_.y, ground + 0.45f);       // don't sink into the sea floor

    // Standing depth reached (e.g. swam onto a beach): walk again
    if (depth < swimDepth - 0.1f && ground + eyeHeight >= surface + floatEye - 0.2f) {
      swimming_ = false;
      jumpVel_ = 0.0f;
      cameraPos_.y = ground + eyeHeight;
    }
  }

  // Head under water?
  float surfaceHere = terrain_.getWaterSurfaceAt(cameraPos_.x, cameraPos_.z, swimTime_);
  underwater_ = cameraPos_.y < surfaceHere - 0.05f;

  // Oxygen: ~20 s of breath, refills quickly at the surface. When empty you are pushed up.
  if (underwater_)
    oxygen_ = std::max(0.0f, oxygen_ - dt / 20.0f);
  else
    oxygen_ = std::min(1.0f, oxygen_ + dt / 3.0f);
}

// ----------------- Runtime config API -----------------
void Engine::regenerateTerrain() {
  terrain_.generateProcedural(terrainSize_, terrainScale_, heightScale_,
                              textureTile_, terrainParams_);
  placePlayerOnLand();
}

void Engine::placePlayerOnLand() {
  // Start on a beach looking out to sea. Walk outward from the centre in 16 directions
  // to the coast, step back inland a little, and keep the lowest spot (a beach, not a cliff).
  float waterY = terrain_.getWaterY();
  float half = terrain_.getHalfExtent();
  swimming_ = underwater_ = jumping_ = false;
  oxygen_ = 1.0f;
  float bestH = 1e9f;
  cameraPos_ = glm::vec3(0.0f, terrain_.getHeightAt(0, 0) + 1.7f, 0.0f);
  for (int k = 0; k < 16; ++k) {
    float a = 0.8f + k * 0.3927f;
    glm::vec2 dir(cos(a), sin(a));
    float lastLand = -1.0f;
    for (float r = 0.0f; r < half * 0.95f; r += 2.0f) {
      float h = terrain_.getHeightAt(dir.x * r, dir.y * r);
      if (h > waterY + 0.8f) lastLand = r;
      else if (lastLand >= 0.0f && r - lastLand > 30.0f) break; // reached open water
    }
    if (lastLand < 0.0f) continue;
    float r = std::max(0.0f, lastLand - 12.0f);
    float x = dir.x * r, z = dir.y * r;
    float h = terrain_.getHeightAt(x, z);
    if (h > waterY + 1.0f && h < bestH) {
      bestH = h;
      cameraPos_ = glm::vec3(x, h + 1.7f, z);
      yaw_ = glm::degrees(a);
      pitch_ = -6.0f;
    }
  }
}


