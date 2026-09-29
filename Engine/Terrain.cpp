#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>
#include <GLFW/glfw3.h>
#include "libs/stb_image.h"

Terrain::~Terrain() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (flatVBO_) glDeleteBuffers(1, &flatVBO_);
    if (flatEBO_) glDeleteBuffers(1, &flatEBO_);
    if (flatVAO_) glDeleteVertexArrays(1, &flatVAO_);
    if (flatTex_) glDeleteTextures(1, &flatTex_);
    if (heightTex_) glDeleteTextures(1, &heightTex_);
    if (waterVBO_) glDeleteBuffers(1, &waterVBO_);
    if (waterVAO_) glDeleteVertexArrays(1, &waterVAO_);
}

void Terrain::buildProcedural(const std::vector<float>& interleaved, const std::vector<unsigned int>& indices) {
    // Cleanup flat if switching
    isFlat_ = false;
    if (flatVAO_) { glDeleteVertexArrays(1, &flatVAO_); flatVAO_ = 0; }
    if (flatVBO_) { glDeleteBuffers(1, &flatVBO_); flatVBO_ = 0; }
    if (flatEBO_) { glDeleteBuffers(1, &flatEBO_); flatEBO_ = 0; }

    if (vao_) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_) { glDeleteBuffers(1, &vbo_); }
    if (ebo_) { glDeleteBuffers(1, &ebo_); }
    glGenVertexArrays(1, &vao_); glGenBuffers(1, &vbo_); glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float), interleaved.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    indexCount_ = (unsigned int)indices.size();

    GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

bool Terrain::buildFlat(const std::string& texturePath) {
    // Cleanup procedural if switching
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (ebo_) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }

    // Load texture externally ideally; here, assume texture loaded elsewhere and bound at draw time.
    // We'll create quad buffers only; texture ID must be set via external binding.

    struct FlatVertex { glm::vec3 pos; glm::vec3 normal; glm::vec2 uv; };
    // Use tile_ to improve texture quality by repeating across the large plane
    FlatVertex verts[4] = {
        { {-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f,      0.0f} },
        { { 1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {tile_,     0.0f} },
        { { 1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {tile_,     tile_} },
        { {-1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f,      tile_} },
    };
    unsigned int indices[6] = { 0, 1, 2, 0, 2, 3 };

    if (flatVAO_) { glDeleteVertexArrays(1, &flatVAO_); }
    if (flatVBO_) { glDeleteBuffers(1, &flatVBO_); }
    if (flatEBO_) { glDeleteBuffers(1, &flatEBO_); }
    glGenVertexArrays(1, &flatVAO_);
    glGenBuffers(1, &flatVBO_);
    glGenBuffers(1, &flatEBO_);
    glBindVertexArray(flatVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, flatVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, flatEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FlatVertex), (void*)offsetof(FlatVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(FlatVertex), (void*)offsetof(FlatVertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(FlatVertex), (void*)offsetof(FlatVertex, uv));
    glBindVertexArray(0);

    // Load texture for flat terrain
    if (flatTex_) { glDeleteTextures(1, &flatTex_); flatTex_ = 0; }
    int w=0,h=0,c=0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(texturePath.c_str(), &w, &h, &c, 0);
    if (data) {
        glGenTextures(1, &flatTex_);
        glBindTexture(GL_TEXTURE_2D, flatTex_);
        GLenum format = (c == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Try anisotropic filtering if supported for better quality at grazing angles
        GLfloat maxAniso = 0.0f;
        glGetFloatv(0x84FF /*GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT*/, &maxAniso);
        if (maxAniso > 1.0f) {
            GLfloat aniso = glm::min(8.0f, maxAniso);
            glTexParameterf(GL_TEXTURE_2D, 0x84FE /*GL_TEXTURE_MAX_ANISOTROPY_EXT*/, aniso);
        }
        stbi_image_free(data);
    }
    isFlat_ = true;
    return true;
}

bool Terrain::loadProceduralTexture(const std::string& texturePath) {
    if (procTexture_) { glDeleteTextures(1, &procTexture_); procTexture_ = 0; }
    int w=0,h=0,c=0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(texturePath.c_str(), &w, &h, &c, 0);
    if (!data) return false;
    glGenTextures(1, &procTexture_);
    glBindTexture(GL_TEXTURE_2D, procTexture_);
    GLenum format = (c == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLfloat maxAniso = 0.0f;
    glGetFloatv(0x84FF /*GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT*/, &maxAniso);
    if (maxAniso > 1.0f) {
        GLfloat aniso = glm::min(8.0f, maxAniso);
        glTexParameterf(GL_TEXTURE_2D, 0x84FE /*GL_TEXTURE_MAX_ANISOTROPY_EXT*/, aniso);
    }
    stbi_image_free(data);
    return true;
}

void Terrain::draw(GLuint shaderProgram, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos) {
    glUseProgram(shaderProgram);
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, &cameraPos.x);
    if (isFlat_) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, flatTex_);
        glm::mat4 M = model * glm::scale(glm::mat4(1.0f), flatScale_);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, &(proj * view * M)[0][0]);
        glBindVertexArray(flatVAO_);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    } else {
        const TerrainParams& P = params_;
        auto U = [&](const char* n) { return glGetUniformLocation(shaderProgram, n); };
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, procTexture_);
        glUniform1i(U("shadeMode"), 1);
        glUniform1f(U("waterY"), P.waterEnabled ? waterY_ : -1e9f);
        glUniform1f(U("beachWidth"), P.beachWidth * heightScale_);
        glUniform1f(U("rockSlope"), P.rockSlope);
        glUniform1f(U("snowLine"), P.snowLine * heightScale_);
        glUniform1f(U("snowBlend"), std::max(P.snowBlend * heightScale_, 0.01f));
        glUniform3fv(U("grassTint"), 1, &P.grassTint.x);
        glUniform3fv(U("sandColor"), 1, &P.sandColor.x);
        glUniform3fv(U("rockColor"), 1, &P.rockColor.x);
        glUniform3fv(U("snowColor"), 1, &P.snowColor.x);
        glUniform3fv(U("fogColor"), 1, &P.fogColor.x);
        glUniform1f(U("fogDensity"), P.fogDensity);
        glUniformMatrix4fv(U("model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(U("mvp"), 1, GL_FALSE, &(proj * view * model)[0][0]);
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        if (P.waterEnabled) drawWater(shaderProgram, model, view, proj);
        glUniform1i(U("shadeMode"), 0); // objects/coins drawn afterwards use plain shading
    }
}

void Terrain::drawWater(GLuint shaderProgram, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj) {
    if (!waterVAO_) return;
    const TerrainParams& P = params_;
    auto U = [&](const char* n) { return glGetUniformLocation(shaderProgram, n); };
    glm::mat4 M = glm::translate(model, glm::vec3(0.0f, waterY_, 0.0f));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, heightTex_);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(U("heightTex"), 2);
    glUniform1i(U("shadeMode"), 2);
    glUniform1f(U("time"), (float)glfwGetTime());
    glUniform1f(U("terrainHalf"), (size_ - 1) * 0.5f * scale_);
    glUniform1f(U("waterOpacity"), P.waterOpacity);
    glUniform3fv(U("waterShallow"), 1, &P.waterShallow.x);
    glUniform3fv(U("waterDeep"), 1, &P.waterDeep.x);
    glUniformMatrix4fv(U("model"), 1, GL_FALSE, &M[0][0]);
    glUniformMatrix4fv(U("mvp"), 1, GL_FALSE, &(proj * view * M)[0][0]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(waterVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}
// --- Presets ---
static const char* kPresetNames[] = { "Mountains", "Rolling Hills", "Islands", "Plains", "Mesa / Canyons", "Alpine Peaks" };

const char* const* TerrainParams::presetNames(int& count) {
    count = (int)(sizeof(kPresetNames) / sizeof(kPresetNames[0]));
    return kPresetNames;
}

TerrainParams TerrainParams::preset(int id) {
    TerrainParams p; // defaults are the "Mountains" look
    switch (id) {
    case 1: // Rolling hills
        p.frequency = 0.003f; p.octaves = 5; p.ridgeAmount = 0.05f; p.warpStrength = 40.0f;
        p.heightPower = 1.1f; p.waterLevel = 0.18f; p.snowLine = 2.0f; p.rockSlope = 0.45f;
        p.erosionIterations = 40000; break;
    case 2: // Islands
        p.frequency = 0.006f; p.ridgeAmount = 0.35f; p.heightPower = 1.4f; p.islandStrength = 1.0f;
        p.islandRadius = 0.35f; p.waterLevel = 0.28f; p.snowLine = 0.95f; p.beachWidth = 0.035f; break;
    case 3: // Plains
        p.frequency = 0.0025f; p.octaves = 4; p.ridgeAmount = 0.0f; p.warpStrength = 20.0f;
        p.heightPower = 1.0f; p.waterLevel = 0.12f; p.snowLine = 3.0f; p.rockSlope = 0.6f;
        p.erosionIterations = 20000; break;
    case 4: // Mesa / canyons
        p.frequency = 0.004f; p.ridgeAmount = 0.2f; p.heightPower = 1.3f; p.terraceStrength = 0.9f;
        p.terraceSteps = 6; p.waterLevel = 0.05f; p.snowLine = 3.0f; p.rockSlope = 0.2f;
        p.sandColor = {0.82f, 0.55f, 0.35f}; p.rockColor = {0.62f, 0.34f, 0.24f};
        p.grassTint = {1.0f, 0.85f, 0.55f}; p.fogColor = {0.85f, 0.75f, 0.65f}; break;
    case 5: // Alpine
        p.frequency = 0.0055f; p.ridgeAmount = 1.0f; p.heightPower = 2.0f; p.warpStrength = 80.0f;
        p.waterLevel = 0.15f; p.snowLine = 0.55f; p.snowBlend = 0.12f; p.erosionIterations = 120000; break;
    default: break;
    }
    return p;
}

// --- Noise helpers (seeded gradient/Perlin noise) ---
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static inline float fade5(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
static inline float smooth01(float e0, float e1, float x) {
    float t = glm::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f); return t * t * (3.0f - 2.0f * t);
}
static inline unsigned hash2(int x, int y, unsigned seed) {
    unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16);
}
static float gradNoise(float x, float y, unsigned seed) {
    int xi = (int)std::floor(x), yi = (int)std::floor(y);
    float xf = x - xi, yf = y - yi;
    auto g = [&](int ix, int iy, float dx, float dy) {
        float a = (hash2(ix, iy, seed) & 1023u) * (6.2831853f / 1024.0f);
        return std::cos(a) * dx + std::sin(a) * dy;
    };
    float u = fade5(xf), v = fade5(yf);
    float n = lerpf(lerpf(g(xi, yi, xf, yf), g(xi + 1, yi, xf - 1, yf), u),
                    lerpf(g(xi, yi + 1, xf, yf - 1), g(xi + 1, yi + 1, xf - 1, yf - 1), u), v);
    return n * 1.4142f; // roughly [-1, 1]
}

namespace {
struct HeightGen {
    const TerrainParams& p; unsigned seed; float ox, oy;
    explicit HeightGen(const TerrainParams& params) : p(params), seed((unsigned)params.seed) {
        ox = (hash2(1, 7, seed) & 0xffff) * 0.37f; oy = (hash2(9, 3, seed) & 0xffff) * 0.37f;
    }
    float fbm(float x, float y) const {
        float total = 0, amp = 1, freq = 1, norm = 0;
        for (int i = 0; i < p.octaves; ++i) {
            total += amp * gradNoise(x * freq, y * freq, seed + i * 101u); norm += amp;
            amp *= p.persistence; freq *= p.lacunarity;
        }
        return total / norm; // ~[-1,1]
    }
    // Ridged multifractal: sharp crests, with detail concentrated on the slopes
    float ridged(float x, float y) const {
        float total = 0, amp = 1, freq = 1, norm = 0, weight = 1;
        for (int i = 0; i < p.octaves; ++i) {
            float n = 1.0f - std::fabs(gradNoise(x * freq, y * freq, seed + 977u + i * 131u));
            n *= n; n *= weight;
            weight = glm::clamp(n * 2.0f, 0.0f, 1.0f);
            total += n * amp; norm += amp; amp *= p.persistence; freq *= p.lacunarity;
        }
        return total / norm; // ~[0,1]
    }
    // Raw (un-normalized) height at grid coordinate
    float raw(float gx, float gz) const {
        float x = gx + ox, y = gz + oy;
        if (p.warpStrength > 0.0f) {
            float wx = gradNoise(x * p.frequency * 2.0f + 5.2f, y * p.frequency * 2.0f + 1.3f, seed + 11u);
            float wy = gradNoise(x * p.frequency * 2.0f + 9.7f, y * p.frequency * 2.0f + 3.1f, seed + 23u);
            x += wx * p.warpStrength; y += wy * p.warpStrength;
        }
        float fx = x * p.frequency, fy = y * p.frequency;
        float base = fbm(fx, fy) * 0.5f + 0.5f;
        float ridge = ridged(fx, fy);
        // Ridges appear only in some regions so the land has both plains and ranges
        float mask = smooth01(0.30f, 0.70f, gradNoise(fx * 0.35f + 40.0f, fy * 0.35f + 40.0f, seed + 55u) * 0.5f + 0.5f);
        return lerpf(base, ridge, p.ridgeAmount * (0.35f + 0.65f * mask));
    }
};
} // namespace

// Simple particle-based hydraulic erosion (carves valleys, deposits sediment)
static void erode(std::vector<float>& h, int N, const TerrainParams& P) {
    if (P.erosionIterations <= 0) return;
    std::mt19937 rng((unsigned)P.seed * 2654435761u + 17u);
    std::uniform_real_distribution<float> U(0.0f, 1.0f);
    const float inertia = 0.05f, capacityK = 4.0f, minSlope = 0.01f, evaporate = 0.012f, gravity = 4.0f;
    const int maxLife = 40, R = 3;

    // Precompute brush (erosion is spread over a small disk to avoid pits)
    std::vector<int> bx, bz; std::vector<float> bw; float wsum = 0;
    for (int dz = -R; dz <= R; ++dz) for (int dx = -R; dx <= R; ++dx) {
        float d = std::sqrt((float)(dx * dx + dz * dz)); if (d > R) continue;
        bx.push_back(dx); bz.push_back(dz); bw.push_back(1.0f - d / R); wsum += 1.0f - d / R;
    }
    for (auto& w : bw) w /= wsum;

    auto sample = [&](float x, float z, float& gx, float& gz) {
        int xi = (int)x, zi = (int)z; float fx = x - xi, fz = z - zi;
        float h00 = h[zi * N + xi], h10 = h[zi * N + xi + 1], h01 = h[(zi + 1) * N + xi], h11 = h[(zi + 1) * N + xi + 1];
        gx = (h10 - h00) * (1 - fz) + (h11 - h01) * fz;
        gz = (h01 - h00) * (1 - fx) + (h11 - h10) * fx;
        return h00 * (1 - fx) * (1 - fz) + h10 * fx * (1 - fz) + h01 * (1 - fx) * fz + h11 * fx * fz;
    };

    for (int it = 0; it < P.erosionIterations; ++it) {
        float x = U(rng) * (N - 3) + 1, z = U(rng) * (N - 3) + 1;
        float dx = 0, dz = 0, speed = 1, water = 1, sediment = 0;
        for (int life = 0; life < maxLife; ++life) {
            int xi = (int)x, zi = (int)z;
            if (xi < 1 || zi < 1 || xi >= N - 2 || zi >= N - 2) break;
            float fx = x - xi, fz = z - zi, gx, gz;
            float hOld = sample(x, z, gx, gz);
            dx = dx * inertia - gx * (1 - inertia); dz = dz * inertia - gz * (1 - inertia);
            float len = std::sqrt(dx * dx + dz * dz);
            if (len < 1e-6f) { float a = U(rng) * 6.2831853f; dx = std::cos(a); dz = std::sin(a); len = 1; }
            dx /= len; dz /= len;
            x += dx; z += dz;
            if ((int)x < 1 || (int)z < 1 || (int)x >= N - 2 || (int)z >= N - 2) break;
            float gx2, gz2; float hNew = sample(x, z, gx2, gz2);
            float dh = hNew - hOld;
            float cap = std::max(-dh * speed * water * capacityK, minSlope);
            if (sediment > cap || dh > 0) {
                float amt = (dh > 0) ? std::min(dh, sediment) : (sediment - cap) * P.depositStrength;
                sediment -= amt;
                h[zi * N + xi] += amt * (1 - fx) * (1 - fz); h[zi * N + xi + 1] += amt * fx * (1 - fz);
                h[(zi + 1) * N + xi] += amt * (1 - fx) * fz; h[(zi + 1) * N + xi + 1] += amt * fx * fz;
            } else {
                float amt = std::min((cap - sediment) * P.erosionStrength, -dh);
                for (size_t k = 0; k < bw.size(); ++k) {
                    int cx = xi + bx[k], cz = zi + bz[k];
                    if (cx < 0 || cz < 0 || cx >= N || cz >= N) continue;
                    float take = amt * bw[k]; float& c = h[cz * N + cx];
                    float t = std::min(take, c); c -= t; sediment += t;
                }
            }
            speed = std::sqrt(std::max(0.0f, speed * speed + (-dh) * gravity));
            water *= (1 - evaporate);
        }
    }
}

// Thermal erosion: slopes steeper than the talus angle shed material downhill
static void thermal(std::vector<float>& h, int N, int passes, float talus) {
    for (int pass = 0; pass < passes; ++pass) {
        for (int z = 1; z < N - 1; ++z) for (int x = 1; x < N - 1; ++x) {
            float c = h[z * N + x]; float maxD = 0; int mi = -1;
            static const int off[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            for (int k = 0; k < 4; ++k) {
                float d = c - h[(z + off[k][1]) * N + x + off[k][0]];
                if (d > maxD) { maxD = d; mi = k; }
            }
            if (mi >= 0 && maxD > talus) {
                float mv = (maxD - talus) * 0.4f;
                h[z * N + x] -= mv; h[(z + off[mi][1]) * N + x + off[mi][0]] += mv;
            }
        }
    }
}

void Terrain::generateProcedural(int size, float scale, float heightScale, float textureTile) {
    generateProcedural(size, scale, heightScale, textureTile, params_);
}

void Terrain::generateProcedural(int size, float scale, float heightScale, float textureTile, const TerrainParams& params) {
    size_ = std::max(size, 4); scale_ = scale; heightScale_ = heightScale; tile_ = textureTile; params_ = params;
    const int N = size_;
    const TerrainParams& P = params_;
    float half = (N - 1) * 0.5f * scale_;

    // 1. Raw noise, normalized to 0..1 so the settings behave the same at any seed
    HeightGen gen(P);
    std::vector<float> h(N * N);
    float lo = 1e9f, hi = -1e9f;
    for (int z = 0; z < N; ++z) for (int x = 0; x < N; ++x) {
        float v = gen.raw((float)x, (float)z); h[z * N + x] = v; lo = std::min(lo, v); hi = std::max(hi, v);
    }
    float range = std::max(hi - lo, 1e-6f);

    // 2. Shaping: redistribution curve, terraces, island falloff
    for (int z = 0; z < N; ++z) for (int x = 0; x < N; ++x) {
        float v = (h[z * N + x] - lo) / range;
        v = std::pow(glm::clamp(v, 0.0f, 1.0f), std::max(P.heightPower, 0.05f));
        if (P.terraceStrength > 0.0f && P.terraceSteps > 1) {
            float s = v * P.terraceSteps, f = std::floor(s);
            float stepped = (f + smooth01(0.35f, 0.65f, s - f)) / P.terraceSteps;
            v = lerpf(v, stepped, glm::clamp(P.terraceStrength, 0.0f, 1.0f));
        }
        if (P.islandStrength > 0.0f) {
            float nx = (x / (float)(N - 1)) * 2 - 1, nz = (z / (float)(N - 1)) * 2 - 1;
            float d = std::sqrt(nx * nx + nz * nz);
            float fall = 1.0f - smooth01(P.islandRadius, 1.0f, d);
            v *= lerpf(1.0f, fall, glm::clamp(P.islandStrength, 0.0f, 1.0f));
        }
        h[z * N + x] = v;
    }

    // 3. Erosion (in normalized units)
    thermal(h, N, P.thermalIterations, 0.6f / N * 3.0f);
    erode(h, N, P);

    // 4. To world space
    heights_.resize(N * N);
    for (int i = 0; i < N * N; ++i) heights_[i] = h[i] * heightScale_;
    waterY_ = P.waterLevel * heightScale_;

    auto H = [&](int x, int z) { return heights_[glm::clamp(z, 0, N - 1) * N + glm::clamp(x, 0, N - 1)]; };

    // 5. Mesh with smooth normals from central differences
    std::vector<float> inter; inter.reserve((size_t)N * N * 8);
    for (int z = 0; z < N; ++z) for (int x = 0; x < N; ++x) {
        float dhdx = (H(x + 1, z) - H(x - 1, z)) / (2.0f * scale_);
        float dhdz = (H(x, z + 1) - H(x, z - 1)) / (2.0f * scale_);
        glm::vec3 n = glm::normalize(glm::vec3(-dhdx, 1.0f, -dhdz));
        inter.insert(inter.end(), { x * scale_ - half, H(x, z), z * scale_ - half, n.x, n.y, n.z,
                                    (float)x / (N - 1) * tile_, (float)z / (N - 1) * tile_ });
    }
    std::vector<unsigned int> idx; idx.reserve((size_t)(N - 1) * (N - 1) * 6);
    for (int z = 0; z < N - 1; ++z) for (int x = 0; x < N - 1; ++x) {
        unsigned tl = z * N + x, tr = tl + 1, bl = (z + 1) * N + x, br = bl + 1;
        idx.insert(idx.end(), { tl, bl, br, tl, br, tr });
    }
    buildProcedural(inter, idx);

    // 6. Height texture (used by the water shader for depth / shoreline foam)
    if (!heightTex_) glGenTextures(1, &heightTex_);
    glBindTexture(GL_TEXTURE_2D, heightTex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, N, N, 0, GL_RED, GL_FLOAT, heights_.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 7. Water quad (much larger than the terrain so it reads as an ocean)
    if (!waterVAO_) {
        glGenVertexArrays(1, &waterVAO_); glGenBuffers(1, &waterVBO_);
        glBindVertexArray(waterVAO_); glBindBuffer(GL_ARRAY_BUFFER, waterVBO_);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }
    float e = half * 8.0f;
    float quad[] = { -e, 0, -e, 0, 1, 0, 0, 0,   e, 0, -e, 0, 1, 0, 1, 0,   e, 0, e, 0, 1, 0, 1, 1,
                     -e, 0, -e, 0, 1, 0, 0, 0,   e, 0, e, 0, 1, 0, 1, 1,   -e, 0, e, 0, 1, 0, 0, 1 };
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
}

float Terrain::getHeightAt(float wx, float wz) const {
    if (isFlat_ || heights_.empty()) return 0.0f; // flat plane at Y=0
    const int N = size_;
    float half = (N - 1) * 0.5f * scale_;
    float x = glm::clamp((wx + half) / scale_, 0.0f, (float)(N - 1) - 0.001f);
    float z = glm::clamp((wz + half) / scale_, 0.0f, (float)(N - 1) - 0.001f);
    int xi = (int)x, zi = (int)z; float fx = x - xi, fz = z - zi;
    float h = lerpf(lerpf(heights_[zi * N + xi], heights_[zi * N + xi + 1], fx),
                    lerpf(heights_[(zi + 1) * N + xi], heights_[(zi + 1) * N + xi + 1], fx), fz);
    // The player wades through shallow water instead of walking on the sea floor
    if (params_.waterEnabled) h = std::max(h, waterY_ - 0.6f);
    return h;
}
