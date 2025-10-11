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

    // Make it a direction in view space, then transform to world space
    vec4 worldDir4 = invView * vec4(eye.xyz, 0.0);
    vDir = normalize(worldDir4.xyz);

    // we still must output a clip position
    gl_Position = vec4(aPos, 0.0, 1.0);
}
