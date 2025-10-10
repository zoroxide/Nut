#include "Nut.h"

// STB Image
#define STB_IMAGE_IMPLEMENTATION
#include "./libs/stb_image.h"

// GLMs
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STLs
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>

// Settings (kept as constants)
const int TERRAIN_SIZE = 256;
const float TERRAIN_SCALE = 1.0f;
const float HEIGHT_SCALE = 6.0f;
const float TEXTURE_TILE = 22.0f;

// Vertex struct
struct Vertex { glm::vec3 pos; glm::vec3 normal; glm::vec2 uv; };

// Static instance pointer
Engine* Engine::s_instance_ = nullptr;

Engine::Engine()
    : window_(nullptr), shaderProgram_(0), vao_(0), vbo_(0), ebo_(0), indexCount_(0), grassTexture_(0),
      cameraPos_(0.0f, 6.0f, 12.0f), yaw_(-90.0f), pitch_(-15.0f), mouseSensitivity_(0.12f), moveSpeed_(6.0f),
      lastX_(0.0), lastY_(0.0), firstMouse_(true), lastFrame_(Clock::now()), deltaTime_(0.0f), jumping_(false), jumpVel_(0.0f), vsyncEnabled_(true)
{
    std::fill(std::begin(keys_), std::end(keys_), false);
    s_instance_ = this;
}

Engine::~Engine() {
    // Cleanup
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    if (grassTexture_) glDeleteTextures(1, &grassTexture_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (window_) glfwTerminate();
}

bool Engine::init(bool fullscreen) {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = nullptr;
    int SCR_W = 1280, SCR_H = 720;
    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        SCR_W = mode->width; SCR_H = mode->height;
    }

    window_ = glfwCreateWindow(SCR_W, SCR_H, "Procedural Terrain (Engine)", monitor, nullptr);
    if (!window_) { glfwTerminate(); return false; }

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

    // Resources
    shaderProgram_ = createProgram("Nut/shaders/vertex.glsl", "Nut/shaders/fragment.glsl");
    buildTerrainMesh();
    uploadMeshToGPU();

    return true;
}

void Engine::vsync(bool enabled) {
    vsyncEnabled_ = enabled;
    if (window_) glfwSwapInterval(enabled ? 1 : 0);
}

void Engine::load_terrain_using_texture(const std::string &path) {
    grassTexture_ = loadTexture(path.c_str());
    if (!grassTexture_) std::cerr << "Warning: grass texture failed to load\n";
}

void Engine::mainloop() {
    if (!window_) return;

    // Set some uniforms that don't change often
    glUseProgram(shaderProgram_);
    glUniform3f(glGetUniformLocation(shaderProgram_, "lightDir"), -0.2f, -1.0f, -0.3f);
    glUniform3f(glGetUniformLocation(shaderProgram_, "lightColor"), 1.0f, 0.98f, 0.9f);
    glUniform1i(glGetUniformLocation(shaderProgram_, "texture1"), 0);
    glUniform3f(glGetUniformLocation(shaderProgram_, "fogColor"), 0.53f, 0.8f, 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram_, "fogDensity"), 0.008f);

    int SCR_W, SCR_H;
    glfwGetWindowSize(window_, &SCR_W, &SCR_H);

    lastFrame_ = Clock::now();
    while (!glfwWindowShouldClose(window_)) {
        // Timing
        auto now = Clock::now();
        deltaTime_ = std::chrono::duration<float>(now - lastFrame_).count();
        lastFrame_ = now;
        updateMovement(deltaTime_);

        // Camera
        glm::vec3 front(
            cos(glm::radians(yaw_)) * cos(glm::radians(pitch_)),
            sin(glm::radians(pitch_)),
            sin(glm::radians(yaw_)) * cos(glm::radians(pitch_))
        );

        glm::mat4 view = glm::lookAt(cameraPos_, cameraPos_ + glm::normalize(front), glm::vec3(0,1,0));
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)SCR_W / (float)SCR_H, 0.1f, 500.0f);
        glm::mat4 model(1.0f);

        // Sky
        glDisable(GL_DEPTH_TEST);
        glUseProgram(shaderProgram_);
        glUniform1i(glGetUniformLocation(shaderProgram_, "renderSky"), 1);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glEnable(GL_DEPTH_TEST);

        // Clear
        glClearColor(0.53f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw terrain
        glUseProgram(shaderProgram_);
        glUniform1i(glGetUniformLocation(shaderProgram_, "renderSky"), 0);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "mvp"), 1, GL_FALSE, glm::value_ptr(proj * view * model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(shaderProgram_, "viewPos"), 1, glm::value_ptr(cameraPos_));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTexture_);
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, (GLsizei)indexCount_, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }
}

// ---------------- Utility / helpers ----------------

std::string Engine::loadFile(const char* path) {
    std::ifstream in(path);
    if(!in) { std::cerr << "Failed to open " << path << std::endl; return {}; }
    std::stringstream ss; ss << in.rdbuf(); return ss.str();
}

GLuint Engine::compileShaderFromFile(const char* path, GLenum type) {
    std::string src = loadFile(path);
    const char* csrc = src.c_str();
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &csrc, nullptr);
    glCompileShader(sh);
    GLint ok; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if(!ok) { char buf[1024]; glGetShaderInfoLog(sh, 1024, nullptr, buf); std::cerr << "Shader compile error (" << path << "):\n" << buf << std::endl; }
    return sh;
}

GLuint Engine::createProgram(const char* vsPath, const char* fsPath) {
    GLuint vs = compileShaderFromFile(vsPath, GL_VERTEX_SHADER);
    GLuint fs = compileShaderFromFile(fsPath, GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram(); glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if(!ok) { char buf[1024]; glGetProgramInfoLog(prog, 1024, nullptr, buf); std::cerr << "Program link error:\n" << buf << std::endl; }
    glDeleteShader(vs); glDeleteShader(fs); return prog;
}

inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float fade(float t) { return t * t * (3.0f - 2.0f * t); }

int hashI(int x, int y) { int n = x + y * 57; n = (n << 13) ^ n; return (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff; }
float valueNoise(int x, int y) { return (hashI(x, y) / float(0x7fffffff)) * 2.0f - 1.0f; }
float smoothNoise(float x, float y) {
    int xf = (int)floor(x); int yf = (int)floor(y);
    float xf_frac = x - xf; float yf_frac = y - yf;
    float v00 = valueNoise(xf, yf); float v10 = valueNoise(xf + 1, yf); float v01 = valueNoise(xf, yf + 1); float v11 = valueNoise(xf + 1, yf + 1);
    float i1 = lerp(v00, v10, fade(xf_frac)); float i2 = lerp(v01, v11, fade(xf_frac)); return lerp(i1, i2, fade(yf_frac));
}

float Engine::fbm(float x, float y) {
    float total = 0.0f; float amp = 1.0f; float freq = 1.0f; const int OCT = 6; const float gain = 0.5f;
    for (int i = 0; i < OCT; ++i) { total += amp * smoothNoise(x * freq, y * freq); freq *= 2.0f; amp *= gain; }
    return total;
}

float Engine::getTerrainHeight(float wx, float wz) {
    float half = (TERRAIN_SIZE - 1) * 0.5f * TERRAIN_SCALE;
    float x = (wx + half) / TERRAIN_SCALE;
    float z = (wz + half) / TERRAIN_SCALE;
    return fbm(x * 0.06f, z * 0.06f) * HEIGHT_SCALE;
}

void Engine::buildTerrainMesh() {
    std::vector<Vertex> vertices; std::vector<GLuint> indices;
    int N = TERRAIN_SIZE; float half = (N - 1) * 0.5f * TERRAIN_SCALE;
    std::vector<std::vector<float>> heights(N, std::vector<float>(N));
    for (int z = 0; z < N; ++z) for (int x = 0; x < N; ++x) heights[z][x] = fbm(x * 0.06f, z * 0.06f) * HEIGHT_SCALE;

    vertices.resize(N * N);
    for (int z = 0; z < N; ++z) for (int x = 0; x < N; ++x) {
        Vertex &V = vertices[z * N + x];
        V.pos = glm::vec3(x * TERRAIN_SCALE - half, heights[z][x], z * TERRAIN_SCALE - half);
        V.uv  = glm::vec2((float)x / (N - 1) * TEXTURE_TILE, (float)z / (N - 1) * TEXTURE_TILE);
    }

    for (int z = 0; z < N - 1; ++z) for (int x = 0; x < N - 1; ++x) {
        int tl = z * N + x; int tr = tl + 1; int bl = (z + 1) * N + x; int br = bl + 1;
        if (tl < 0 || tr < 0 || bl < 0 || br < 0) continue;
        if (tl >= N*N || tr >= N*N || bl >= N*N || br >= N*N) continue;
        if (tl == bl || bl == br || br == tl) continue;
        if (tl == br || br == tr || tr == tl) continue;
        indices.push_back(tl); indices.push_back(bl); indices.push_back(br);
        indices.push_back(tl); indices.push_back(br); indices.push_back(tr);
    }

    std::vector<glm::vec3> normalSum(vertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i]; unsigned int i1 = indices[i + 1]; unsigned int i2 = indices[i + 2];
        glm::vec3 p0 = vertices[i0].pos; glm::vec3 p1 = vertices[i1].pos; glm::vec3 p2 = vertices[i2].pos;
        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        normalSum[i0] += normal; normalSum[i1] += normal; normalSum[i2] += normal;
    }
    for (size_t i = 0; i < vertices.size(); ++i) vertices[i].normal = glm::normalize(normalSum[i]);

    // Upload to member buffers (interleave here)
    // Cleanup old
    if (vao_) { glDeleteBuffers(1, &vbo_); glDeleteBuffers(1, &ebo_); glDeleteVertexArrays(1, &vao_); }
    glGenVertexArrays(1, &vao_); glGenBuffers(1, &vbo_); glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);

    std::vector<float> inter; inter.reserve(vertices.size() * 8);
    for (auto &v : vertices) inter.insert(inter.end(), {v.pos.x, v.pos.y, v.pos.z, v.normal.x, v.normal.y, v.normal.z, v.uv.x, v.uv.y});

    glBindBuffer(GL_ARRAY_BUFFER, vbo_); glBufferData(GL_ARRAY_BUFFER, inter.size() * sizeof(float), inter.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_); glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    indexCount_ = indices.size();

    GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void Engine::uploadMeshToGPU() {
    // Mesh upload is integrated into buildTerrainMesh for simplicity in this refactor
}

GLuint Engine::loadTexture(const char* path) {
    int width, height, nrChannels; stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (!data) { std::cerr << "Failed to load texture: " << path << std::endl; return 0; }
    GLuint textureID; glGenTextures(1, &textureID); glBindTexture(GL_TEXTURE_2D, textureID);
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return textureID;
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
    if (key >= 0 && key < 1024) keys_[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window_, true);
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !jumping_) { jumping_ = true; jumpVel_ = 7.0f; }
}

void Engine::updateMovement(float dt) {
    glm::vec3 front = glm::normalize(glm::vec3(cos(glm::radians(yaw_)), 0.0f, sin(glm::radians(yaw_))));
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));
    float sp = moveSpeed_ * dt; if (keys_[GLFW_KEY_LEFT_SHIFT]) sp *= 1.9f;
    glm::vec3 move(0.0f); if (keys_[GLFW_KEY_W]) move += front * sp; if (keys_[GLFW_KEY_S]) move -= front * sp; if (keys_[GLFW_KEY_A]) move -= right * sp; if (keys_[GLFW_KEY_D]) move += right * sp;
    cameraPos_ += move;
    float terrainY = getTerrainHeight(cameraPos_.x, cameraPos_.z); float eyeHeight = 1.7f;
    if (jumping_) {
        cameraPos_.y += jumpVel_ * dt; jumpVel_ -= 18.0f * dt;
        if (cameraPos_.y <= terrainY + eyeHeight) { cameraPos_.y = terrainY + eyeHeight; jumping_ = false; jumpVel_ = 0.0f; }
    } else {
        cameraPos_.y = terrainY + eyeHeight;
    }
}
