// OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// STB Image
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

// GLMs
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STLs
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <chrono>

// Timing
using Clock = std::chrono::high_resolution_clock;

// ---------- Settings ----------  //
const int TERRAIN_SIZE = 256;      // vertices per side
const float TERRAIN_SCALE = 1.0f;  // world spacing between vertices
const float HEIGHT_SCALE = 6.0f;   // amplitude of terrain
const float TEXTURE_TILE = 22.0f;  // how many times the grass texture tiles across the terrain

// ---------- Globals ---------- //
GLFWwindow* window = nullptr;
GLuint shaderProgram = 0;
GLuint vao = 0, vbo = 0, ebo = 0;
size_t indexCount = 0;
GLuint grassTexture = 0;

// camera / fps
glm::vec3 cameraPos(0.0f, 6.0f, 12.0f);
float yaw = -90.0f, pitch = -15.0f;
float mouseSensitivity = 0.12f;
float moveSpeed = 6.0f;

// mouse
double lastX = 0.0, lastY = 0.0;
bool firstMouse = true;

// timing
auto lastFrame = Clock::now();
float deltaTime = 0.0f;

// input
bool keys[1024] = {false};
bool jumping = false;
float jumpVel = 0.0f;

// ---------- Utility ---------- //

// Forward declaration for fbm
float fbm(float x, float y);

// Sample terrain height at world (x, z) using the same noise as mesh
float getTerrainHeight(float wx, float wz) {
    // Convert world x,z to heightmap coordinates
    float half = (TERRAIN_SIZE - 1) * 0.5f * TERRAIN_SCALE;
    float x = (wx + half) / TERRAIN_SCALE;
    float z = (wz + half) / TERRAIN_SCALE;
    return fbm(x * 0.06f, z * 0.06f) * HEIGHT_SCALE;
}

// Load entire file into a string
std::string loadFile(const char* path) {
    std::ifstream in(path);
    if(!in) {
        std::cerr << "Failed to open " << path << std::endl;
        return {};
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// ---------- Shader ---------- //

// Compile shader from preset file (frag, vert)
GLuint compileShaderFromFile(const char* path, GLenum type) {
    std::string src = loadFile(path);
    const char* csrc = src.c_str();
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &csrc, nullptr);
    glCompileShader(sh);
    GLint ok;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if(!ok) {
        char buf[1024];
        glGetShaderInfoLog(sh, 1024, nullptr, buf);
        std::cerr << "Shader compile error (" << path << "):\n" << buf << std::endl;
    }
    return sh;
}

// Create shader program from vertex and fragment shader files
GLuint createProgram(const char* vsPath, const char* fsPath) {
    GLuint vs = compileShaderFromFile(vsPath, GL_VERTEX_SHADER);
    GLuint fs = compileShaderFromFile(fsPath, GL_FRAGMENT_SHADER);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if(!ok) {
        char buf[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, buf);
        std::cerr << "Program link error:\n" << buf << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

// ---------- Terrain noise (perlin noise algorithm) ---------- //

inline float lerp(float a, float b, float t) { return a + (b - a) * t; } // linear interpolation
inline float fade(float t) { return t * t * (3.0f - 2.0f * t); }         // fade function for interpolation

// Hash function to get a repeatable random value for integer coordinates
int hashI(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
}

// 2D value noise
float valueNoise(int x, int y) {
    return (hashI(x, y) / float(0x7fffffff)) * 2.0f - 1.0f;
}

// Smooth noise by bilinear interpolation
float smoothNoise(float x, float y) {
    // Integer coordinates
    int xf = (int)floor(x);
    int yf = (int)floor(y);

    // Fractional parts
    float xf_frac = x - xf;
    float yf_frac = y - yf;

    // Get noise values at the corners
    float v00 = valueNoise(xf, yf);
    float v10 = valueNoise(xf + 1, yf);
    float v01 = valueNoise(xf, yf + 1);
    float v11 = valueNoise(xf + 1, yf + 1);

    // Bilinear interpolation
    float i1 = lerp(v00, v10, fade(xf_frac));
    float i2 = lerp(v01, v11, fade(xf_frac));

    // Final interpolation
    return lerp(i1, i2, fade(yf_frac));
}

// Fractal Brownian Motion (fbm) using multiple octaves of smooth noise
float fbm(float x, float y) {
    // Initial values
    float total = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    const int OCT = 6;
    const float gain = 0.5f;

    // Sum octaves
    for (int i = 0; i < OCT; ++i) {
        total += amp * smoothNoise(x * freq, y * freq);
        freq *= 2.0f;
        amp *= gain;
    }

    // return value in range [-1, 1] (normalized)
    return total;
}

// ---------- Terrain mesh (with UVs) ---------- //

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

std::vector<Vertex> vertices;
std::vector<GLuint> indices;

// Build terrain mesh with proper normals and UVs
void buildTerrainMesh() {
    // Clear old data
    vertices.clear();
    indices.clear();

    // Terrain parameters
    int N = TERRAIN_SIZE;
    float half = (N - 1) * 0.5f * TERRAIN_SCALE;

    // --- Precompute heights --- //
    std::vector<std::vector<float>> heights(N, std::vector<float>(N));
    for (int z = 0; z < N; ++z)
        for (int x = 0; x < N; ++x)
            heights[z][x] = fbm(x * 0.06f, z * 0.06f) * HEIGHT_SCALE;

    // --- Generate vertices --- //
    vertices.resize(N * N);
    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            Vertex &V = vertices[z * N + x];
            V.pos = glm::vec3(x * TERRAIN_SCALE - half, heights[z][x], z * TERRAIN_SCALE - half);
            V.uv  = glm::vec2((float)x / (N - 1) * TEXTURE_TILE, (float)z / (N - 1) * TEXTURE_TILE);
        }
    }

    // --- Generate indices (fully connected grid) --- //
    for (int z = 0; z < N - 1; ++z) {
        for (int x = 0; x < N - 1; ++x) {
            // Indices of the quad corners
            int tl = z * N + x;
            int tr = tl + 1;
            int bl = (z + 1) * N + x;
            int br = bl + 1;

            // Ensure all indices are within bounds
            if (tl < 0 || tr < 0 || bl < 0 || br < 0) continue;
            if (tl >= N*N || tr >= N*N || bl >= N*N || br >= N*N) continue;

            // Skip degenerate triangles
            if (tl == bl || bl == br || br == tl) continue;
            if (tl == br || br == tr || tr == tl) continue;

            // Triangle 1: top-left, bottom-left, bottom-right (CCW)
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(br);

            // Triangle 2: top-left, bottom-right, top-right (CCW)
            indices.push_back(tl);
            indices.push_back(br);
            indices.push_back(tr);
        }
    }

    // --- Calculate normals properly --- //
    std::vector<glm::vec3> normalSum(vertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        // Get triangle vertex indices
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        // Get triangle vertex positions
        glm::vec3 p0 = vertices[i0].pos;
        glm::vec3 p1 = vertices[i1].pos;
        glm::vec3 p2 = vertices[i2].pos;

        // Compute face normal
        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

        // Accumulate normals for each vertex
        normalSum[i0] += normal;
        normalSum[i1] += normal;
        normalSum[i2] += normal;
    }

    // Normalize the summed normals to get the vertex normals
    for (size_t i = 0; i < vertices.size(); ++i) vertices[i].normal = glm::normalize(normalSum[i]);
}

// ---------- Upload to GPU ---------- //
void uploadMeshToGPU() {
    // Cleanup old buffers
    if (vao) {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
    }

    // Create and bind VAO, VBO, EBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    // Bind VAO
    glBindVertexArray(vao);

    // Interleave vertex data: pos(3), normal(3), uv(2)
    std::vector<float> inter;
    inter.reserve(vertices.size() * 8);
    for (auto &v : vertices) {
        inter.insert(inter.end(), {v.pos.x, v.pos.y, v.pos.z, v.normal.x, v.normal.y, v.normal.z, v.uv.x, v.uv.y});
    }

    // Upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, inter.size() * sizeof(float), inter.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    indexCount = indices.size();

    // Set vertex attributes
    GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Unbind VAO
    glBindVertexArray(0);
}

// ---------- Texture ---------- //

// Load texture from file using stb_image
GLuint loadTexture(const char* path) {

    // Load image with stb_image
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    // handle failure
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    // Generate OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Determine format
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Free image memory
    stbi_image_free(data);

    return textureID;
}

// ---------- Input ---------- //

// Mouse movement
void cursorPosCallback(GLFWwindow*, double xpos, double ypos) {

    // First mouse setup
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }

    // Calculate offsets
    double xoff = xpos - lastX;
    double yoff = lastY - ypos;

    // Update last positions
    lastX = xpos; lastY = ypos;
    xoff *= mouseSensitivity;
    yoff *= mouseSensitivity;
    yaw += (float)xoff;
    pitch += (float)yoff;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
}

// Keyboard input
void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    // Close on ESC
    if (key >= 0 && key < 1024) {
        keys[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    // Escape key to close
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Space bar for jump
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !jumping) {
        jumping = true;
        jumpVel = 7.0f; // jump initial velocity (ideal 7)
    }
}

// Update camera position based on input and terrain
void updateMovement(float dt) {

    // FPS movement: move on terrain
    glm::vec3 front = glm::normalize(glm::vec3(
        cos(glm::radians(yaw)),
        0.0f,
        sin(glm::radians(yaw))
    ));

    // Right vector
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));

    // handle move speed
    float sp = moveSpeed * dt;
    if (keys[GLFW_KEY_LEFT_SHIFT]) sp *= 1.9f; // sprint (ideal 2.0f, but 1.9f to avoid floating point issues)

    // Movement vector
    glm::vec3 move(0.0f);
    if (keys[GLFW_KEY_W]) move += front * sp;
    if (keys[GLFW_KEY_S]) move -= front * sp;
    if (keys[GLFW_KEY_A]) move -= right * sp;
    if (keys[GLFW_KEY_D]) move += right * sp;

    // Update camera position
    cameraPos += move;

    // Clamp camera Y to terrain height + eye offset, with jump
    float terrainY = getTerrainHeight(cameraPos.x, cameraPos.z);
    float eyeHeight = 1.7f;

    // Clamp camera Y to terrain height + eye offset
    if (jumping) {

        // Update jump position and velocity
        cameraPos.y += jumpVel * dt;
        jumpVel -= 18.0f * dt; // gravity

        // Clamp jump
        if (cameraPos.y <= terrainY + eyeHeight) {
            // Landed
            cameraPos.y = terrainY + eyeHeight;
            // Reset jump
            jumping = false;
            // Reset jump velocity
            jumpVel = 0.0f;
        }

    } else {
        // Not jumping, just clamp to terrain (and resets when pressing again in mid air)
        // if (cameraPos.y < terrainY + eyeHeight) cameraPos.y = terrainY + eyeHeight;

        // Always reset to terrain height when not jumping
        cameraPos.y = terrainY + eyeHeight;
    }
}

// ---------- Main ---------- //
int main() {
    // Initialize GLFW
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window (fullscreen)
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    // Get the screen resolution
    int SCR_W = mode->width;
    int SCR_H = mode->height;

    // Create a fullscreen window
    window = glfwCreateWindow(SCR_W, SCR_H, "Procedural Terrain (Fullscreen)", monitor, nullptr);

    // handle failure
    if (!window) { glfwTerminate(); return -1; }

    // Initialize GLEW
    glfwMakeContextCurrent(window);

    // Enable V-Sync
    glfwSwapInterval(1); // 0 for no vsync, 1 for vsync
    glewExperimental = GL_TRUE;

    // handle failure
    if (glewInit() != GLEW_OK) return -1;

    // Set input callbacks
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    // OpenGL settings
    // glViewport(0, 0, SCR_W, SCR_H); // windowed mode
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Load resources
    shaderProgram = createProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    buildTerrainMesh();
    uploadMeshToGPU();

    // Load and handle grass texture
    grassTexture = loadTexture("assets/grass/grass.png");
    if (!grassTexture) std::cerr << "Warning: grass texture failed to load\n";

    // Set shader uniforms
    glUseProgram(shaderProgram);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"), -0.2f, -1.0f, -0.3f);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 0.98f, 0.9f);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // Fog and sky panorama uniforms (reduced fog for clarity) - not working from fragment shader
    glUniform3f(glGetUniformLocation(shaderProgram, "fogColor"), 0.53f, 0.8f, 1.0f); // sky blue
    glUniform1f(glGetUniformLocation(shaderProgram, "fogDensity"), 0.008f);
    glUniform1i(glGetUniformLocation(shaderProgram, "renderSky"), 0);

    // Main loop
    lastFrame = Clock::now();
    while (!glfwWindowShouldClose(window)) {
        // Timing
        auto now = Clock::now();
        deltaTime = std::chrono::duration<float>(now - lastFrame).count();
        lastFrame = now;
        updateMovement(deltaTime);

        // Camera matrices
        glm::vec3 front(
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
        );

        // View and projection
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + glm::normalize(front), glm::vec3(0,1,0));
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)SCR_W / (float)SCR_H, 0.1f, 500.0f);
        glm::mat4 model(1.0f);


        // Draw sky panorama (fullscreen quad, no depth test)
        glDisable(GL_DEPTH_TEST);
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "renderSky"), 1);
        // glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        // glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        // glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));

        // Draw a fullscreen triangle (no VAO needed in core profile, but may need to set up a simple VAO if required)
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glEnable(GL_DEPTH_TEST);

        // Clear screen
        glClearColor(0.53f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Draw terrain
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "renderSky"), 0);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(proj * view * model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

        // Bind grass texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, 0);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    // Cleanup
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &grassTexture);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    glfwTerminate();


    return 0;
}
