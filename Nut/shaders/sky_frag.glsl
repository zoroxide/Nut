#version 330 core
out vec4 FragColor;
in vec3 vDir;

uniform sampler2D panorama; // equirectangular panorama bound to texture unit 1
uniform bool hasPanorama;

const float PI = 3.14159265358979323846;

void main() {
    vec3 dir = normalize(vDir);

    if (!hasPanorama) {
        // fallback vertical gradient
        float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 skyTop = vec3(0.53, 0.8, 1.0);
        vec3 skyBottom = vec3(0.85, 0.95, 1.0);
        FragColor = vec4(mix(skyBottom, skyTop, t), 1.0);
        return;
    }

    // Convert direction to equirectangular (u,v)
    float theta = atan(dir.z, dir.x); // -pi..pi
    float phi = asin(clamp(dir.y, -1.0, 1.0)); // -pi/2..pi/2

    float u = (theta / (2.0 * PI)) + 0.5;
    float v = 0.5 - (phi / PI); // flip v as needed to match panorama orientation

    vec3 color = texture(panorama, vec2(u, v)).rgb;
    FragColor = vec4(color, 1.0);
}
