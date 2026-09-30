#pragma once
#include <GL/glew.h>

// Procedurally generated helper textures (created once at startup)
namespace Textures {
// Tileable RGBA noise: r,g,b,a hold fractal noise at increasing frequencies. Mipmapped, repeating.
GLuint createNoise(int size = 256);
// Tileable water ripple detail: rg = normal xz (0.5 centred), b = foam noise, a = height. Mipmapped.
GLuint createWaterDetail(int size = 256);
}
