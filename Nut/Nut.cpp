#include "Nut.h"
#include "gui/gui.h"

// Image loading is encapsulated in subsystems; no STB implementation here

// GLMs
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_inverse.hpp>

// STLs
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Vertex struct
struct Vertex { glm::vec3 pos; glm::vec3 normal; glm::vec2 uv; };

// Static instance pointer
Engine* Engine::s_instance_ = nullptr;

Engine::Engine()
    : window_(nullptr), shaderProgram_(0),
    skyShader_(0),
      cameraPos_(0.0f, 6.0f, 12.0f), yaw_(-90.0f), pitch_(-15.0f), mouseSensitivity_(0.12f), moveSpeed_(6.0f),
      lastX_(0.0), lastY_(0.0), firstMouse_(true), lastFrame_(Clock::now()), deltaTime_(0.0f), jumping_(false), jumpVel_(0.0f), vsyncEnabled_(true)
{
    std::fill(std::begin(keys_), std::end(keys_), false);
    s_instance_ = this;

    // Defaults for configurable constants and paths
    terrainSize_ = 512;
    terrainScale_ = 1.0f;
    heightScale_ = 6.0f;
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
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    if (skyShader_) glDeleteProgram(skyShader_);
    // Skybox VAO/VBO are managed by Skybox class
    if (window_) glfwTerminate();

    if (gui_) { delete gui_; gui_ = nullptr; }
}

bool Engine::init(bool fullscreen) {
    // glfw init
    if (!glfwInit()) return false;

    // glfw window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWmonitor* monitor = nullptr;
    int SCR_W = 1280, SCR_H = 720;

    // Fullscreen setup
    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        SCR_W = mode->width; SCR_H = mode->height;
    }

    // Create window
    window_ = glfwCreateWindow(SCR_W, SCR_H, "Procedural Terrain (Engine)", monitor, nullptr);
    if (!window_) { glfwTerminate(); return false; }

    // GLEW + GL context
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(vsyncEnabled_ ? 1 : 0);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return false;

    // Input
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window_, Engine::cursorPosCallbackStatic);
    glfwSetKeyCallback(window_, Engine::keyCallbackStatic);

    // GL settings
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Resources Loading(shaders, terrain mesh, etc)
    shaderProgram_ = shaders_.loadProgram("terrain", "Nut/shaders/vertex.glsl", "Nut/shaders/fragment.glsl");
    // Reuse terrain shader initially for simple model rendering.
    // For robust rendering, a separate model shader can be added later.

    // Create sky shader and setup Skybox helper
    skyShader_ = shaders_.loadProgram("sky", "Nut/shaders/sky_vert.glsl", "Nut/shaders/sky_frag.glsl");
    sky_.setShader(skyShader_);
    sky_.initFullscreenTriangle();

    // Renderer programs and scene wiring
    renderer_.setPrograms(shaderProgram_, skyShader_);
    renderer_.setScene(&terrain_, &sky_, &models_);

    // Generate initial procedural terrain via Terrain subsystem
    terrain_.generateProcedural(terrainSize_, terrainScale_, heightScale_, textureTile_);

    // Initialize GUI after the OpenGL context is created
    if (gui_) gui_->init(window_);

    return true;
}

void Engine::vsync(bool enabled) {
    vsyncEnabled_ = enabled;
    if (window_) glfwSwapInterval(enabled ? 1 : 0);
}

bool Engine::getVsyncEnabled() const {
    return vsyncEnabled_;
}

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
const std::string& Engine::getPanoramaPath() const { return panoramaPath_; }
void Engine::setPanoramaPath(const std::string &p) { panoramaPath_ = p; }
const std::string& Engine::getTerrainTexturePath() const { return terrainTexturePath_; }
void Engine::setTerrainTexturePath(const std::string &p) { terrainTexturePath_ = p; }

// Cloud accessors
bool Engine::getCloudEnabled() const { return cloudEnabled_; }
void Engine::setCloudEnabled(bool v) { cloudEnabled_ = v; }
float Engine::getCloudSpeed() const { return cloudSpeed_; }
void Engine::setCloudSpeed(float v) { cloudSpeed_ = v; }
float Engine::getCloudScale() const { return cloudScale_; }
void Engine::setCloudScale(float v) { cloudScale_ = v; }
float Engine::getCloudOpacity() const { return cloudOpacity_; }
void Engine::setCloudOpacity(float v) { cloudOpacity_ = v; }

void Engine::load_terrain_using_texture(const std::string &texturePath, const std::string &objPath) {
    // Load a procedural terrain texture via Terrain subsystem
    if (!texturePath.empty()) {
        terrain_.loadProceduralTexture(texturePath);
    }

    if (!objPath.empty()) {
        models_.loadOBJ(objPath, glm::vec3(0.0f), glm::vec3(1.0f));
    }
}

void Engine::add_house(const std::string& objPath, const glm::vec3& position, const glm::vec3& scale) {
    // Delegate to Models manager
    if (!models_.loadOBJ(objPath, position, scale)) {
        std::cerr << "Error: Failed to load house OBJ via Models: " << objPath << "\n";
    }
}

bool Engine::load_flat_terrain(const std::string &texturePath) {
    // Propagate current texture tiling to Terrain so flat UVs are repeated
    terrain_.setTextureTile(textureTile_);
    hasFlat_ = terrain_.buildFlat(texturePath);
    if (!hasFlat_) return false;
    terrain_.setFlatScale(glm::vec3(terrainSize_ * terrainScale_ * 0.5f, 1.0f, terrainSize_ * terrainScale_ * 0.5f));
    return true;
}
void Engine::mainloop() {
    // Safety check
    if (!window_) return;

    // Set some uniforms that don't change often (terrain shader)
    glUseProgram(shaderProgram_);
    glUniform3f(glGetUniformLocation(shaderProgram_, "lightDir"), -0.2f, -1.0f, -0.3f);
    glUniform3f(glGetUniformLocation(shaderProgram_, "lightColor"), 1.0f, 0.98f, 0.9f);
    glUniform1i(glGetUniformLocation(shaderProgram_, "texture1"), 0);
    glUniform3f(glGetUniformLocation(shaderProgram_, "fogColor"), 0.53f, 0.8f, 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram_, "fogDensity"), 0.008f);

    // sky shader texture unit binding (skybox cubemap will be bound to unit 1 at render time)
    glUseProgram(skyShader_);
    glUniform1i(glGetUniformLocation(skyShader_, "skybox"), 1);

    // set initial cloud uniforms (time will be updated per-frame)
    glUniform1i(glGetUniformLocation(skyShader_, "cloudEnabled"), cloudEnabled_ ? 1 : 0);
    glUniform1f(glGetUniformLocation(skyShader_, "cloudSpeed"), cloudSpeed_);
    glUniform1f(glGetUniformLocation(skyShader_, "cloudScale"), cloudScale_);
    glUniform1f(glGetUniformLocation(skyShader_, "cloudOpacity"), cloudOpacity_);

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
        updateMovement(deltaTime_);

        // Camera
        // Camera matrices
        camera_.setPosition(cameraPos_);
        camera_.setYawPitch(yaw_, pitch_);
        glm::mat4 view = camera_.getView();
        glm::mat4 proj = camera_.getProj(60.0f, (float)SCR_W / (float)SCR_H, 0.1f, 500.0f);
        glm::mat4 model(1.0f);

        // --- Clear first (important!) ---
        glClearColor(0.53f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Draw sky full-screen triangle ---
        glDisable(GL_DEPTH_TEST);

        // Inverse matrices
        glm::mat4 invProj = glm::inverse(proj);

        // Draw sky
        glUseProgram(skyShader_);
        glUniformMatrix4fv(glGetUniformLocation(skyShader_, "invProj"), 1, GL_FALSE, glm::value_ptr(invProj));

        // Provide full inverse view matrix; the vertex shader uses mat3(invView) so
        // translation is ignored and the sky remains fixed (no parallax from camera position).
        glm::mat4 invView = glm::inverse(view);
        glUniformMatrix4fv(glGetUniformLocation(skyShader_, "invView"), 1, GL_FALSE, glm::value_ptr(invView));
    glUniform1i(glGetUniformLocation(skyShader_, "hasSkybox"), sky_.hasCubemap() ? 1 : 0);

        // update animated uniforms
        // float t = (float)std::chrono::duration<double>(Clock::now() - lastFrame_).count();

        // Use running time since program start for smoother animation
        static double startTime = std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
        float runTime = (float)(std::chrono::duration<double>(Clock::now().time_since_epoch()).count() - startTime);

        glUniform1f(glGetUniformLocation(skyShader_, "time"), runTime);
        glUniform1i(glGetUniformLocation(skyShader_, "cloudEnabled"), cloudEnabled_ ? 1 : 0);
        glUniform1f(glGetUniformLocation(skyShader_, "cloudSpeed"), cloudSpeed_);
        glUniform1f(glGetUniformLocation(skyShader_, "cloudScale"), cloudScale_);
        glUniform1f(glGetUniformLocation(skyShader_, "cloudOpacity"), cloudOpacity_);

        // Bind skybox cubemap handled inside Skybox::draw

    // Draw full-screen triangle via Skybox class
    sky_.draw(invView, invProj, sky_.hasCubemap(), runTime, cloudEnabled_, cloudSpeed_, cloudScale_, cloudOpacity_);

        // --- Re-enable depth test for terrain ---
        glEnable(GL_DEPTH_TEST);

    // --- Then draw terrain via Renderer (which calls Terrain) ---
    renderer_.drawFrame(view, proj, model, invView, invProj, cameraPos_, sky_.hasCubemap(),
                runTime, cloudEnabled_, cloudSpeed_, cloudScale_, cloudOpacity_);

        // --- Draw models via Models manager ---
        models_.drawAll(shaderProgram_, view, proj);

        // Render GUI
        gui_->render();

        // Swap buffers and poll events
        glfwSwapBuffers(window_);
        glfwPollEvents();
    }
}

// ---------------- Utility / helpers ----------------

// Removed low-level shader and file utilities; ShaderManager handles this

// ---------------- Terrain generation inline helpers ----------------
// Removed low-level texture loading; Terrain/Skybox manage textures

bool Engine::panorama(const std::string &path) {
    return sky_.loadFromPath(path);
}


// Input callbacks
void Engine::cursorPosCallbackStatic(GLFWwindow*, double xpos, double ypos) { if (s_instance_) s_instance_->cursorPosCallback(xpos, ypos); }
void Engine::keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods) { if (s_instance_) s_instance_->keyCallback(key, scancode, action, mods); }

void Engine::cursorPosCallback(double xpos, double ypos) {
    if (firstMouse_) { lastX_ = xpos; lastY_ = ypos; firstMouse_ = false; }
    double xoff = xpos - lastX_; double yoff = lastY_ - ypos;
    lastX_ = xpos; lastY_ = ypos; xoff *= mouseSensitivity_; yoff *= mouseSensitivity_; yaw_ += (float)xoff; pitch_ += (float)yoff; pitch_ = glm::clamp(pitch_, -89.0f, 89.0f);
}

void Engine::keyCallback(int key, int, int action, int) {
    if (key >= 0 && key < 1024)
        keys_[key] = (action == GLFW_PRESS || action == GLFW_REPEAT); // key states

    // ESC closes the window
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window_, true);

    // SPACE for jumping
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !jumping_) {
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


void Engine::updateMovement(float dt) {
    // Update camera position based on key states
    // WASD for movement, SPACE for jump (handled in key callback)
    // Simple gravity and jumping mechanics
    // Camera stays at terrain height + eye height when not jumping

    glm::vec3 front = glm::normalize(glm::vec3(cos(glm::radians(yaw_)), 0.0f, sin(glm::radians(yaw_))));
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));

    // sprinting
    float sp = moveSpeed_ * dt; if (keys_[GLFW_KEY_LEFT_SHIFT]) sp *= SPRINT_MULTIPLIER; // sprint multiplier (ideal 1.9 for normal sprint)
    glm::vec3 move(0.0f); if (keys_[GLFW_KEY_W]) move += front * sp; if (keys_[GLFW_KEY_S]) move -= front * sp; if (keys_[GLFW_KEY_A]) move -= right * sp; if (keys_[GLFW_KEY_D]) move += right * sp;
    cameraPos_ += move;

    // Terrain collision and gravity
    float terrainY = terrain_.getHeightAt(cameraPos_.x, cameraPos_.z); float eyeHeight = 1.7f;
    if (jumping_) {
        cameraPos_.y += jumpVel_ * dt; jumpVel_ -= 18.0f * dt;
        if (cameraPos_.y <= terrainY + eyeHeight) { cameraPos_.y = terrainY + eyeHeight; jumping_ = false; jumpVel_ = 0.0f; }
    } else {
        cameraPos_.y = terrainY + eyeHeight;
    }
}

// ----------------- Runtime config API -----------------
void Engine::regenerateTerrain() {
    terrain_.generateProcedural(terrainSize_, terrainScale_, heightScale_, textureTile_);
}
