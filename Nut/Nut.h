#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <chrono>

// forward-declare GUI class (defined in Nut/gui)
class GUI;

using Clock = std::chrono::high_resolution_clock;

class Engine {
public:
    Engine();
    ~Engine();

    // Initialize the engine and create a window. Returns true on success.
    // If fullscreen is true, a fullscreen window is created.
    bool init(bool fullscreen = true);

    // Load terrain texture from path and bind it for rendering.
    void load_terrain_using_texture(const std::string &path);

    // Enable or disable VSync (must be called after init or will be applied on next init)
    void vsync(bool enabled);

    // Enter the main loop and run until window close.
    void mainloop();

private:
    // Internal state (opaque to users)
    GLFWwindow* window_;
    GLuint shaderProgram_;
    GLuint vao_, vbo_, ebo_;
    size_t indexCount_;
    GLuint grassTexture_;
    GLuint panoramaTexture_;

    // Sky renderer
    GLuint skyShader_;
    GLuint skyVAO_, skyVBO_;

    // Camera / movement
    glm::vec3 cameraPos_;
    float yaw_, pitch_;
    float mouseSensitivity_;
    float moveSpeed_;

    // Mouse
    double lastX_, lastY_;
    bool firstMouse_;

    // Timing
    Clock::time_point lastFrame_;
    float deltaTime_;

    // constants
    #define TERRAIN_SIZE 512
    #define TERRAIN_SCALE 1.0f
    #define HEIGHT_SCALE 6.0f
    #define TEXTURE_TILE 22.0f

    // #define NOISE_SCALE 0.1f
    // #define NOISE_OCTAVES 6
    // #define NOISE_PERSISTENCE 0.5f
    // #define NOISE_LACUNARITY 2.0f

    #define JUMP_VELOCITY 7.0f

    // #define GRAVITY 18.0f

    #define SPRINT_MULTIPLIER 1.9f

    // Input
    bool keys_[1024];
    bool jumping_;
    float jumpVel_;

    // VSync state
    bool vsyncEnabled_;

    // Instance pointer for static callbacks
    static Engine* s_instance_;

    // GUI manager
    class GUI; // forward
    GUI* gui_;

    // Configurable constants (moved from macros to members so we can change them at runtime)
    int terrainSize_;
    float terrainScale_;
    float heightScale_;
    float textureTile_;

    // Last-used file paths (for UI / serialization)
    std::string panoramaPath_;
    std::string terrainTexturePath_;


public: // Public API
    // Load a panorama (equirectangular) image to be used as the sky. Returns true on success.
    bool panorama(const std::string &path);

    // Regenerate terrain mesh with current constants
    void regenerateTerrain();

    // Getters / setters for configurable constants and file paths
    int getTerrainSize() const;
    void setTerrainSize(int v);
    float getTerrainScale() const;
    void setTerrainScale(float v);
    float getHeightScale() const;
    void setHeightScale(float v);
    float getTextureTile() const;
    void setTextureTile(float v);

    const std::string& getPanoramaPath() const;
    void setPanoramaPath(const std::string &p);
    const std::string& getTerrainTexturePath() const;
    void setTerrainTexturePath(const std::string &p);

    // Internal helpers (defined in engine implementation)
    std::string loadFile(const char* path);
    GLuint compileShaderFromFile(const char* path, GLenum type);
    GLuint createProgram(const char* vsPath, const char* fsPath);
    float fbm(float x, float y);
    float getTerrainHeight(float wx, float wz);
    void buildTerrainMesh();
    void uploadMeshToGPU();
    GLuint loadTexture(const char* path);

    // Input helpers
    static void cursorPosCallbackStatic(GLFWwindow* , double xpos, double ypos);
    static void keyCallbackStatic(GLFWwindow* , int key, int scancode, int action, int mods);
    void cursorPosCallback(double xpos, double ypos);
    void keyCallback(int key, int scancode, int action, int mods);
    void updateMovement(float dt);
};
