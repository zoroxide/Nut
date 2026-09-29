#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;
uniform sampler2D heightTex;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform int useSolidColor;
uniform vec3 solidColor;
uniform bool renderSky;

uniform vec3 fogColor;
uniform float fogDensity;

// 0 = plain (models, coins), 1 = procedural terrain, 2 = water
uniform int shadeMode;
uniform float waterY;
uniform float beachWidth;
uniform float rockSlope;
uniform float snowLine;
uniform float snowBlend;
uniform vec3 grassTint;
uniform vec3 sandColor;
uniform vec3 rockColor;
uniform vec3 snowColor;

uniform float time;
uniform float terrainHalf;
uniform float waterOpacity;
uniform vec3 waterShallow;
uniform vec3 waterDeep;

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}
float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 4; i++) { v += a * vnoise(p); p *= 2.03; a *= 0.5; }
    return v;
}

vec3 applyFog(vec3 color, float dist) {
    float f = 1.0 - exp(-pow(fogDensity * dist, 2.0));
    return mix(color, fogColor, clamp(f, 0.0, 1.0));
}

void main() {
    vec3 light = normalize(-lightDir);
    vec3 viewVec = viewPos - FragPos;
    float dist = length(viewVec);
    vec3 viewDir = viewVec / dist;

    if (shadeMode == 2) {
        // ---- Water ----
        vec2 p = FragPos.xz;
        // Two scrolling noise layers give a cheap animated normal
        vec2 w1 = vec2(fbm(p * 0.35 + time * 0.10), fbm(p * 0.35 - time * 0.08 + 17.0));
        vec2 w2 = vec2(fbm(p * 1.10 - time * 0.22 + 3.0), fbm(p * 1.10 + time * 0.19 + 9.0));
        vec2 d = (w1 - 0.5) * 0.35 + (w2 - 0.5) * 0.18;
        vec3 n = normalize(vec3(d.x, 1.0, d.y));

        // Water depth from the terrain height map
        vec2 huv = FragPos.xz / (2.0 * terrainHalf) + 0.5;
        float depth = 100.0;
        if (huv.x > 0.0 && huv.x < 1.0 && huv.y > 0.0 && huv.y < 1.0)
            depth = waterY - texture(heightTex, huv).r;
        float shore = clamp(depth, 0.0, 100.0);

        vec3 col = mix(waterShallow, waterDeep, smoothstep(0.0, 5.0, shore));
        float fres = pow(1.0 - max(dot(n, viewDir), 0.0), 4.0);
        vec3 sky = mix(fogColor, vec3(0.45, 0.65, 0.95), 0.5);
        col = mix(col, sky, clamp(0.08 + fres * 0.85, 0.0, 1.0));

        vec3 refl = reflect(-light, n);
        col += lightColor * pow(max(dot(viewDir, refl), 0.0), 120.0) * 1.2;

        // Shoreline foam
        float foamEdge = 0.55 + 0.25 * sin(time * 1.2 + p.x * 0.5 + p.y * 0.4);
        float foam = (1.0 - smoothstep(0.0, foamEdge, shore)) * smoothstep(0.35, 0.75, fbm(p * 2.5 + time * 0.3));
        col = mix(col, vec3(1.0), clamp(foam, 0.0, 1.0) * 0.85);

        float alpha = clamp(waterOpacity * smoothstep(0.0, 0.4, shore) + foam * 0.6, 0.0, 1.0);
        alpha = max(alpha, fres * 0.6 * step(0.0, shore));
        FragColor = vec4(applyFog(col, dist), alpha);
        return;
    }

    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, light), 0.0);

    if (shadeMode == 1) {
        // ---- Procedural terrain: material blend by height and slope ----
        float slope = 1.0 - norm.y;
        float n1 = fbm(FragPos.xz * 0.15);
        float n2 = vnoise(FragPos.xz * 1.7);

        // Break up texture tiling with a second, larger, offset sample
        vec3 grass = texture(texture1, TexCoords).rgb;
        vec3 grassFar = texture(texture1, TexCoords * 0.173 + 0.31).rgb;
        grass = grass * mix(vec3(1.0), grassFar * 1.6, 0.45) * grassTint;
        grass *= 0.8 + 0.4 * n1;

        vec3 rock = rockColor * (0.65 + 0.7 * fbm(FragPos.xz * 0.6 + FragPos.y * 0.3));
        float rockMask = smoothstep(rockSlope, rockSlope + 0.14, slope + (n1 - 0.5) * 0.12);

        vec3 sand = sandColor * (0.9 + 0.2 * n2);
        float sandMask = 1.0 - smoothstep(waterY + beachWidth * 0.4, waterY + beachWidth + (n1 - 0.5) * beachWidth, FragPos.y);
        sandMask *= 1.0 - rockMask * 0.8;

        float snowMask = smoothstep(snowLine, snowLine + snowBlend, FragPos.y + (n1 - 0.5) * snowBlend - slope * snowBlend * 1.2);
        snowMask *= 1.0 - smoothstep(0.35, 0.7, slope);

        vec3 base = mix(grass, rock, rockMask);
        base = mix(base, sand, sandMask);
        base = mix(base, snowColor, snowMask);
        // Wet ground darkens near the waterline
        base *= mix(0.65, 1.0, smoothstep(waterY - 0.2, waterY + 0.6, FragPos.y));

        vec3 skyAmb = vec3(0.50, 0.62, 0.80), groundAmb = vec3(0.28, 0.24, 0.20);
        vec3 amb = mix(groundAmb, skyAmb, norm.y * 0.5 + 0.5) * 0.55;
        // Soft wrap lighting keeps shaded slopes from going flat black
        float wrap = clamp((dot(norm, light) + 0.25) / 1.25, 0.0, 1.0);
        vec3 lit = (amb + lightColor * (0.55 * diff + 0.45 * wrap * wrap)) * base;
        float spec = pow(max(dot(viewDir, reflect(-light, norm)), 0.0), 32.0);
        lit += lightColor * spec * 0.25 * snowMask;
        FragColor = vec4(applyFog(lit, dist), 1.0);
        return;
    }

    // ---- Plain shading (models, coins) ----
    vec3 diffuse = diff * lightColor;
    vec3 reflectDir = reflect(-light, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.25 * spec * lightColor;

    vec3 baseColor = (useSolidColor == 1) ? solidColor : texture(texture1, TexCoords).rgb;
    vec3 color = (0.25 + diffuse + specular) * baseColor;
    color = applyFog(color, dist);

    if (renderSky) {
        float t = clamp(viewDir.y * 0.5 + 0.5, 0.0, 1.0);
        color = mix(vec3(0.85, 0.95, 1.0), vec3(0.53, 0.8, 1.0), t);
    }
    FragColor = vec4(color, 1.0);
}
