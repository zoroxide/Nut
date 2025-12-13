#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>
#include "libs/stb_image.h"

Terrain::~Terrain() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (flatVBO_) glDeleteBuffers(1, &flatVBO_);
    if (flatEBO_) glDeleteBuffers(1, &flatEBO_);
    if (flatVAO_) glDeleteVertexArrays(1, &flatVAO_);
    if (flatTex_) glDeleteTextures(1, &flatTex_);
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
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, procTexture_);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, &(proj * view * model)[0][0]);
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

// --- Internal noise helpers ---
static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
static inline float fade(float t) { return t * t * (3.0f - 2.0f * t); }
static int hashI(int x, int y) { int n = x + y * 57; n = (n << 13) ^ n; return (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff; }
static float valueNoise(int x, int y) { return (hashI(x, y) / float(0x7fffffff)) * 2.0f - 1.0f; }
static float smoothNoise(float x, float y) {
    int xf = (int)floor(x); int yf = (int)floor(y);
    float xf_frac = x - xf; float yf_frac = y - yf;
    float v00 = valueNoise(xf, yf); float v10 = valueNoise(xf + 1, yf); float v01 = valueNoise(xf, yf + 1); float v11 = valueNoise(xf + 1, yf + 1);
    float i1 = lerp(v00, v10, fade(xf_frac)); float i2 = lerp(v01, v11, fade(xf_frac)); return lerp(i1, i2, fade(yf_frac));
}
static float fbm(float x, float y) {
    float total = 0.0f; float amp = 1.0f; float freq = 1.0f; const int OCT = 6; const float gain = 0.5f;
    for (int i = 0; i < OCT; ++i) { total += amp * smoothNoise(x * freq, y * freq); freq *= 2.0f; amp *= gain; }
    return total;
}

void Terrain::generateProcedural(int size, float scale, float heightScale, float textureTile) {
    size_ = size; scale_ = scale; heightScale_ = heightScale; tile_ = textureTile;
    int N = size_;
    float half = (N - 1) * 0.5f * scale_;

    std::vector<float> heights(N * N);
    for (int z = 0; z < N; ++z) for (int x = 0; x < N; ++x)
        heights[z * N + x] = fbm(x * 0.06f, z * 0.06f) * heightScale_;

    struct V { glm::vec3 p; glm::vec3 n; glm::vec2 uv; };
    std::vector<V> verts(N * N);
    for (int z = 0; z < N; ++z) for (int x = 0; x < N; ++x) {
        V &v = verts[z * N + x];
        v.p = glm::vec3(x * scale_ - half, heights[z * N + x], z * scale_ - half);
        v.uv = glm::vec2((float)x / (N - 1) * tile_, (float)z / (N - 1) * tile_);
    }

    std::vector<unsigned int> idx; idx.reserve((N - 1) * (N - 1) * 6);
    for (int z = 0; z < N - 1; ++z) for (int x = 0; x < N - 1; ++x) {
        int tl = z * N + x; int tr = tl + 1; int bl = (z + 1) * N + x; int br = bl + 1;
        idx.push_back(tl); idx.push_back(bl); idx.push_back(br);
        idx.push_back(tl); idx.push_back(br); idx.push_back(tr);
    }

    std::vector<glm::vec3> normalSum(verts.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < idx.size(); i += 3) {
        unsigned int i0 = idx[i]; unsigned int i1 = idx[i + 1]; unsigned int i2 = idx[i + 2];
        glm::vec3 p0 = verts[i0].p; glm::vec3 p1 = verts[i1].p; glm::vec3 p2 = verts[i2].p;
        glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        normalSum[i0] += n; normalSum[i1] += n; normalSum[i2] += n;
    }
    for (size_t i = 0; i < verts.size(); ++i) verts[i].n = glm::normalize(normalSum[i]);

    std::vector<float> inter; inter.reserve(verts.size() * 8);
    for (auto &v : verts) { inter.push_back(v.p.x); inter.push_back(v.p.y); inter.push_back(v.p.z);
        inter.push_back(v.n.x); inter.push_back(v.n.y); inter.push_back(v.n.z);
        inter.push_back(v.uv.x); inter.push_back(v.uv.y); }
    buildProcedural(inter, idx);
}

float Terrain::getHeightAt(float wx, float wz) const {
    if (isFlat_) {
        return 0.0f; // flat plane at Y=0
    }
    float half = (size_ - 1) * 0.5f * scale_;
    float x = (wx + half) / scale_;
    float z = (wz + half) / scale_;
    return fbm(x * 0.06f, z * 0.06f) * heightScale_;
}
