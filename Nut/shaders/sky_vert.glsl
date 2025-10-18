#version 330 core
layout(location = 0) in vec2 aPos; // full-screen triangle positions
out vec3 vDir; // world-space ray direction

uniform mat4 invProj;
uniform mat4 invView;

void main() {
    // aPos is in NDC coords: (-1,-1), (3,-1), (-1,3) -> full-screen triangle trick
    vec4 clip = vec4(aPos, 0.0, 1.0);

    // From clip -> eye (view) space (unproject)
    vec4 eye = invProj * clip;
    eye /= eye.w;

    // Use only the rotational part of the inverse view to avoid camera translation
    // affecting the sky (no parallax). mat3(invView) extracts rotation.
    vec3 worldDir = mat3(invView) * eye.xyz;
    vDir = normalize(worldDir);

    // we still must output a clip position
    gl_Position = vec4(aPos, 0.0, 1.0);
}
