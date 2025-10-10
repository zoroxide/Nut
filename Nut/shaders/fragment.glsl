#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
// uniform vec3 fogColor;
// uniform float fogDensity;
uniform bool renderSky;

void main() {
    vec3 norm = normalize(Normal);
    vec3 light = normalize(-lightDir);

    float diff = max(dot(norm, light), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-light, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.25 * spec * lightColor;

    vec3 tex = texture(texture1, TexCoords).rgb;
    vec3 result = (0.25 + diffuse + specular) * tex;

    // Fog calculation (commented out)
    // float dist = length(viewPos - FragPos);
    // float fogFactor = 1.0 - clamp(exp(-fogDensity * dist * dist), 0.0, 1.0);
    // vec3 color = mix(result, fogColor, fogFactor);
    vec3 color = result;

    // Panorama sky: if rendering background, use a vertical gradient
    if (renderSky) {
        float t = clamp(viewDir.y * 0.5 + 0.5, 0.0, 1.0);
        // vec3 skyTop = fogColor;
        vec3 skyTop = vec3(0.53, 0.8, 1.0);
        vec3 skyBottom = vec3(0.85, 0.95, 1.0);
        color = mix(skyBottom, skyTop, t);
    }
    FragColor = vec4(color, 1.0);
}
