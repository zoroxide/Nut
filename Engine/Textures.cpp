#include "Textures.h"
#include <cmath>
#include <cstdint>
#include <vector>

namespace {
// Periodic value noise: the lattice wraps every `period` cells so the texture tiles seamlessly
float lattice(int x, int y, int period, uint32_t seed) {
    x = ((x % period) + period) % period;
    y = ((y % period) + period) % period;
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return ((h ^ (h >> 16)) & 0xffff) / 65535.0f;
}

float periodicNoise(float x, float y, int period, uint32_t seed) {
    int xi = (int)std::floor(x), yi = (int)std::floor(y);
    float fx = x - xi, fy = y - yi;
    float u = fx * fx * fx * (fx * (fx * 6 - 15) + 10), v = fy * fy * fy * (fy * (fy * 6 - 15) + 10);
    float a = lattice(xi, yi, period, seed), b = lattice(xi + 1, yi, period, seed);
    float c = lattice(xi, yi + 1, period, seed), d = lattice(xi + 1, yi + 1, period, seed);
    return (a + (b - a) * u) * (1 - v) + (c + (d - c) * u) * v;
}

float periodicFbm(float x, float y, int basePeriod, int octaves, uint32_t seed) {
    float sum = 0, amp = 0.5f, norm = 0;
    int period = basePeriod;
    for (int o = 0; o < octaves; ++o) {
        sum += amp * periodicNoise(x * period, y * period, period, seed + o * 31u);
        norm += amp; amp *= 0.5f; period *= 2;
    }
    return sum / norm;
}

GLuint upload(const std::vector<unsigned char>& px, int size) {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return t;
}

unsigned char toByte(float v) { v = v < 0 ? 0 : (v > 1 ? 1 : v); return (unsigned char)(v * 255.0f + 0.5f); }
} // namespace

GLuint Textures::createNoise(int size) {
    std::vector<unsigned char> px((size_t)size * size * 4);
    for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
        float u = x / (float)size, v = y / (float)size;
        unsigned char* p = &px[((size_t)y * size + x) * 4];
        // Stretch the contrast a bit: fbm values cluster around 0.5
        for (int c = 0; c < 4; ++c) {
            float n = periodicFbm(u, v, 4 << c, 4, 11u + c * 101u);
            p[c] = toByte((n - 0.5f) * 1.8f + 0.5f);
        }
    }
    return upload(px, size);
}

GLuint Textures::createWaterDetail(int size) {
    std::vector<float> h((size_t)size * size);
    for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
        float u = x / (float)size, v = y / (float)size;
        // Mix of rounded ripples; ridged component gives sharper wave crests
        float a = periodicFbm(u, v, 8, 5, 7u);
        float r = 1.0f - std::fabs(periodicFbm(u, v, 6, 4, 19u) * 2.0f - 1.0f);
        h[(size_t)y * size + x] = a * 0.6f + r * r * 0.4f;
    }

    std::vector<unsigned char> px((size_t)size * size * 4);
    for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
        auto H = [&](int xx, int yy) { return h[(size_t)((yy + size) % size) * size + (xx + size) % size]; };
        float dx = (H(x + 1, y) - H(x - 1, y)) * size * 0.05f;
        float dy = (H(x, y + 1) - H(x, y - 1)) * size * 0.05f;
        float len = std::sqrt(dx * dx + dy * dy + 1.0f);
        unsigned char* p = &px[((size_t)y * size + x) * 4];
        p[0] = toByte(-dx / len * 0.5f + 0.5f);
        p[1] = toByte(-dy / len * 0.5f + 0.5f);
        p[2] = toByte(periodicFbm(x / (float)size, y / (float)size, 16, 4, 77u) * 1.6f - 0.3f);
        p[3] = toByte(H(x, y));
    }

    return upload(px, size);
}
