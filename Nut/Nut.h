#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <chrono>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>

#include "gui/gui.h"
#include "libs/imgui/imgui.h"

using Clock = std::chrono::high_resolution_clock;

class Engine {
public:
    Engine();
    ~Engine();

    // Initialize the engine and create a window. Returns true on success.
    // If fullscreen is true, a fullscreen window is created.
    bool init(bool fullscreen = true);

    // Load terrain texture and optional OBJ model from paths.
    void load_terrain_using_texture(const std::string &texturePath, const std::string &objPath = "");
    // Load and prepare a simple flat terrain (textured quad). Returns true on success.
    bool load_flat_terrain(const std::string &texturePath);

    // Enable or disable VSync (must be called after init or will be applied on next init)
    void vsync(bool enabled);
    bool getVsyncEnabled() const;

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

    // for locking/unlocking cursor
    bool cursorEnabled_ = false;


    // Instance pointer for static callbacks
    static Engine* s_instance_;

    // GUI manager
    GUI* gui_;

    // Configurable constants (moved from macros to members so we can change them at runtime)
    int terrainSize_;
    float terrainScale_;
    float heightScale_;
    float textureTile_;

    // Last-used file paths (for UI / serialization)
    std::string panoramaPath_;
    std::string terrainTexturePath_;
    
    // Cloud layer settings
    bool cloudEnabled_;
    float cloudSpeed_;
    float cloudScale_;
    float cloudOpacity_;


public: // Public API
    // Load a skybox cubemap. 'path' is either:
    //  - a directory containing right/left/top/bottom/front/back images (.png or .bmp), or
    //  - a single equirectangular image (.png or .bmp)
    // Returns true on success; if path is empty, disables skybox (fallback gradient).
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

    // File path accessors
    const std::string& getPanoramaPath() const;
    void setPanoramaPath(const std::string &p);
    const std::string& getTerrainTexturePath() const;
    void setTerrainTexturePath(const std::string &p);

    // Cloud accessors
    bool getCloudEnabled() const;
    void setCloudEnabled(bool v);
    float getCloudSpeed() const;
    void setCloudSpeed(float v);
    float getCloudScale() const;
    void setCloudScale(float v);
    float getCloudOpacity() const;
    void setCloudOpacity(float v);

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

    // Model data
    // Simple house model representation
    struct House {
        GLuint vao = 0;
        GLuint vbo = 0;
        size_t vertexCount = 0;
        glm::vec3 position{0,0,0};
        glm::vec3 scale{1,1,1};
        float enterRadius = 2.0f; // distance threshold for entering
    };
    std::vector<House> houses_;
    bool insideHouse_ = false;

    // API to add a house model
public:
    void add_house(const std::string& objPath, const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f));

private:
    // Flat terrain resources
    GLuint flatVAO_ = 0;
    GLuint flatVBO_ = 0;
    GLuint flatEBO_ = 0;
    GLuint flatTex_ = 0;
    bool hasFlat_ = false;
};
