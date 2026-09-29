#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <string>
#include <vector>

// All user-tweakable settings for procedural terrain generation and shading.
struct TerrainParams {
    // --- Shape ---
    int   seed = 1337;
    float frequency = 0.0045f;   // base noise frequency (per vertex)
    int   octaves = 7;
    float persistence = 0.5f;    // amplitude falloff per octave
    float lacunarity = 2.0f;     // frequency gain per octave
    float ridgeAmount = 0.6f;    // 0 = smooth rolling hills, 1 = sharp mountain ridges
    float warpStrength = 60.0f;  // domain warping (organic, twisting shapes)
    float heightPower = 1.6f;    // >1 flattens valleys and sharpens peaks
    float terraceStrength = 0.0f;// 0..1 blend towards stepped (mesa) terrain
    int   terraceSteps = 8;
    float islandStrength = 0.0f; // 0..1 fade edges into the sea
    float islandRadius = 0.55f;  // where the fade starts (0..1)

    // --- Erosion ---
    int   erosionIterations = 80000; // hydraulic erosion droplets (0 = off)
    float erosionStrength = 0.3f;
    float depositStrength = 0.3f;
    int   thermalIterations = 6;     // talus smoothing passes (0 = off)

    // --- Water ---
    bool  waterEnabled = true;
    float waterLevel = 0.30f;    // fraction of heightScale
    float waterOpacity = 0.85f;
    glm::vec3 waterShallow{0.10f, 0.55f, 0.60f};
    glm::vec3 waterDeep{0.02f, 0.13f, 0.28f};

    // --- Surface materials (fractions of heightScale) ---
    float beachWidth = 0.025f;
    float rockSlope = 0.32f;     // slope (1 - normal.y) where rock takes over
    float snowLine = 0.72f;
    float snowBlend = 0.10f;
    glm::vec3 grassTint{0.85f, 1.0f, 0.75f};
    glm::vec3 sandColor{0.80f, 0.72f, 0.50f};
    glm::vec3 rockColor{0.42f, 0.38f, 0.35f};
    glm::vec3 snowColor{0.95f, 0.97f, 1.0f};

    // --- Atmosphere ---
    float fogDensity = 0.0022f;  // 0 disables fog
    glm::vec3 fogColor{0.66f, 0.78f, 0.90f};

    // Preset ids: 0 Mountains, 1 Rolling Hills, 2 Islands, 3 Plains, 4 Mesa/Canyons, 5 Alpine
    static TerrainParams preset(int id);
    static const char* const* presetNames(int& count);
};

// Simple Terrain facade to encapsulate procedural and flat terrain
class Terrain {
public:
    Terrain() = default;
    ~Terrain();

    // Build a procedural mesh using provided buffers (interleaved pos/normal/uv)
    // Ownership of VAO/VBO/EBO stays in Terrain.
    void buildProcedural(const std::vector<float>& interleaved, const std::vector<unsigned int>& indices);

    // Build a flat quad terrain and load its texture
    bool buildFlat(const std::string& texturePath);

    // Load a 2D texture to use for procedural terrain
    bool loadProceduralTexture(const std::string& texturePath);

    // High-level: generate a procedural terrain mesh using internal noise
    void generateProcedural(int size, float scale, float heightScale, float textureTile);
    void generateProcedural(int size, float scale, float heightScale, float textureTile, const TerrainParams& params);

    // Query height at world x,z based on current procedural params
    float getHeightAt(float wx, float wz) const;

    // World-space Y of the water surface (or -1e9 when water is disabled)
    float getWaterY() const { return params_.waterEnabled ? waterY_ : -1e9f; }

    // Draw currently configured terrain using given shader and matrices
    void draw(GLuint shaderProgram, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos);

    // Configure tiling and scale for flat draw
    void setFlatScale(const glm::vec3& s) { flatScale_ = s; }
         void setTextureTile(float t) { tile_ = t; }
         float getTextureTile() const { return tile_; }

    // State
    bool isFlat() const { return isFlat_; }

private:
    // Procedural
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    unsigned int indexCount_ = 0;
    GLuint procTexture_ = 0; // optional

    // Flat
    GLuint flatVAO_ = 0, flatVBO_ = 0, flatEBO_ = 0;
    GLuint flatTex_ = 0;
    glm::vec3 flatScale_{1.0f,1.0f,1.0f};
    bool isFlat_ = false;

    // Height map (world-space Y per vertex) kept for queries and water shading
    std::vector<float> heights_;
    TerrainParams params_;
    float waterY_ = 0.0f;
    GLuint heightTex_ = 0;
    GLuint waterVAO_ = 0, waterVBO_ = 0;
    void drawWater(GLuint shaderProgram, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj);

    // Config for procedural generation
    int size_ = 512;
    float scale_ = 1.0f;
    float heightScale_ = 6.0f;
    float tile_ = 22.0f;
};
