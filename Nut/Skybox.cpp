#include "Skybox.h"
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include "libs/stb_image.h"

Skybox::~Skybox() {
    if (skyVBO_) glDeleteBuffers(1, &skyVBO_);
    if (skyVAO_) glDeleteVertexArrays(1, &skyVAO_);
}

bool Skybox::setCubemap(GLuint texID) {
    cubemap_ = texID; return true;
}

void Skybox::setShader(GLuint prog) { skyShader_ = prog; }

void Skybox::initFullscreenTriangle() {
    float skyVerts[] = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
    };
    glGenVertexArrays(1, &skyVAO_);
    glGenBuffers(1, &skyVBO_);
    glBindVertexArray(skyVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVerts), skyVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Skybox::draw(const glm::mat4& invView, const glm::mat4& invProj, bool hasSkybox, float time, bool cloudEnabled, float cloudSpeed, float cloudScale, float cloudOpacity) {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(skyShader_);
    glUniformMatrix4fv(glGetUniformLocation(skyShader_, "invProj"), 1, GL_FALSE, glm::value_ptr(invProj));
    glUniformMatrix4fv(glGetUniformLocation(skyShader_, "invView"), 1, GL_FALSE, glm::value_ptr(invView));
    glUniform1i(glGetUniformLocation(skyShader_, "hasSkybox"), hasSkybox ? 1 : 0);
    glUniform1f(glGetUniformLocation(skyShader_, "time"), time);
    glUniform1i(glGetUniformLocation(skyShader_, "cloudEnabled"), cloudEnabled ? 1 : 0);
    glUniform1f(glGetUniformLocation(skyShader_, "cloudSpeed"), cloudSpeed);
    glUniform1f(glGetUniformLocation(skyShader_, "cloudScale"), cloudScale);
    glUniform1f(glGetUniformLocation(skyShader_, "cloudOpacity"), cloudOpacity);
    if (cubemap_) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_); }
    glBindVertexArray(skyVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

bool Skybox::loadFromPath(const std::string& path) {
    // Delete previous
    if (cubemap_) { glDeleteTextures(1, &cubemap_); cubemap_ = 0; }

    if (path.empty()) return true;

    namespace fs = std::filesystem;
    std::error_code ec;
    bool isDir = fs::is_directory(path, ec);

    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
    stbi_set_flip_vertically_on_load(false);

    if (isDir) {
        auto join = [](const std::string& dir, const std::string& name) -> std::string {
            if (dir.empty()) return name;
            char last = dir.back();
            if (last == '/' || last == '\\') return dir + name;
            return dir + "/" + name;
        };

        std::vector<std::string> faceNames = {"right", "left", "top", "bottom", "front", "back"};
        std::vector<std::string> exts = {".png", ".PNG", ".bmp", ".BMP"};

        int width = 0, height = 0, channels = 0;
        for (GLuint i = 0; i < faceNames.size(); i++) {
            std::string foundPath;
            for (const auto& ext : exts) {
                std::string candidate = join(path, faceNames[i] + ext);
                if (fs::exists(candidate)) { foundPath = candidate; break; }
            }
            if (foundPath.empty()) { glDeleteTextures(1, &texID); return false; }

            unsigned char* data = stbi_load(foundPath.c_str(), &width, &height, &channels, 0);
            if (!data) { glDeleteTextures(1, &texID); return false; }
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
    } else {
        // Single equirectangular image -> convert to cubemap faces on CPU
        int iw = 0, ih = 0, ic = 0;
        unsigned char* img = stbi_load(path.c_str(), &iw, &ih, &ic, 3);
        if (!img) { glDeleteTextures(1, &texID); return false; }

        auto sampleEquirect = [&](float u, float v) -> glm::vec3 {
            u = glm::fract(u);
            v = glm::clamp(v, 0.0f, 1.0f);
            float x = u * (iw - 1);
            float y = v * (ih - 1);
            int xi = glm::clamp((int)x, 0, iw - 1);
            int yi = glm::clamp((int)y, 0, ih - 1);
            int idx = (yi * iw + xi) * 3;
            return glm::vec3(img[idx] / 255.0f, img[idx + 1] / 255.0f, img[idx + 2] / 255.0f);
        };

        auto dirToUV = [&](const glm::vec3& d) -> glm::vec2 {
            float lon = atan2f(d.z, d.x);
            float lat = asinf(glm::clamp(d.y, -1.0f, 1.0f));
            float u = (lon + glm::pi<float>()) / (2.0f * glm::pi<float>());
            float v = (lat + glm::half_pi<float>()) / glm::pi<float>();
            return glm::vec2(u, v);
        };

        auto faceDir = [&](int face, float u, float v) -> glm::vec3 {
            float a = 2.0f * u - 1.0f;
            float b = 2.0f * v - 1.0f;
            switch (face) {
                case 0: return glm::normalize(glm::vec3(1, -b, -a));   // +X right
                case 1: return glm::normalize(glm::vec3(-1, -b, a));   // -X left
                case 2: return glm::normalize(glm::vec3(a, 1, b));     // +Y top
                case 3: return glm::normalize(glm::vec3(a, -1, -b));   // -Y bottom
                case 4: return glm::normalize(glm::vec3(a, -b, 1));    // +Z front
                default:return glm::normalize(glm::vec3(-a, -b, -1));  // -Z back
            }
        };

        int faceSize = std::max(256, iw / 2);
        std::vector<unsigned char> facePixels(faceSize * faceSize * 3);
        for (int face = 0; face < 6; ++face) {
            for (int y = 0; y < faceSize; ++y) {
                for (int x = 0; x < faceSize; ++x) {
                    float u = (x + 0.5f) / faceSize;
                    float v = (y + 0.5f) / faceSize;
                    glm::vec3 dir = faceDir(face, u, v);
                    glm::vec2 uv = dirToUV(dir);
                    glm::vec3 c = sampleEquirect(uv.x, uv.y);
                    int idx = (y * faceSize + x) * 3;
                    facePixels[idx + 0] = (unsigned char)glm::clamp(c.r * 255.0f, 0.0f, 255.0f);
                    facePixels[idx + 1] = (unsigned char)glm::clamp(c.g * 255.0f, 0.0f, 255.0f);
                    facePixels[idx + 2] = (unsigned char)glm::clamp(c.b * 255.0f, 0.0f, 255.0f);
                }
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB, faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, facePixels.data());
        }
        stbi_image_free(img);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    cubemap_ = texID;
    return true;
}
