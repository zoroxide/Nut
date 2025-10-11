#version 330 core
out vec4 FragColor;
in vec3 vDir;

uniform sampler2D panorama;
uniform bool hasPanorama;

void main() {
    vec3 dir = normalize(vDir);

    if (!hasPanorama) {
        float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 skyTop = vec3(0.53, 0.8, 1.0);
        vec3 skyBottom = vec3(0.85, 0.95, 1.0);
        FragColor = vec4(mix(skyBottom, skyTop, t), 1.0);
        return;
    }

    // Equirectangular mapping
    float theta = atan(-dir.z, dir.x);                     
    float phi = asin(clamp(dir.y, -1.0, 1.0));

    float u = (theta / (2.0 * 3.141592653589793)) + 0.5;
    float v = 0.5 + (phi / 3.141592653589793);             

    vec3 color = texture(panorama, vec2(u, v)).rgb;
    FragColor = vec4(color, 1.0);
}
