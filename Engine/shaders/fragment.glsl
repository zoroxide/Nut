#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;         // models, or a custom grass texture for the terrain
uniform sampler2DArray matAlbedo;   // 0 grass, 1 grass2, 2 rock, 3 sand, 4 snow
uniform sampler2DArray matNormal;
uniform int hasMaterials;
uniform int customGrass;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform int useSolidColor;
uniform vec3 solidColor;
uniform float time;

uniform vec3 fogColor;
uniform float fogDensity;

// 0 = plain (models), 1 = procedural terrain
uniform int shadeMode;
uniform float waterY;
uniform float beachWidth;
uniform float rockSlope;
uniform float snowLine;
uniform float snowBlend;
uniform float texScale;
uniform vec3 grassTint;
uniform vec3 sandTint;
uniform vec3 rockTint;
uniform vec3 snowTint;
uniform vec3 waterShallow;
uniform vec3 waterDeep;

// Camera below the ocean surface: everything is seen through water
uniform int underwater;
uniform vec3 uwColor;

// Panorama (optional): used for fog colour and ambient light
uniform samplerCube skyTex;
uniform int hasSky;
uniform mat3 skyRot;
uniform float skyExposure;
uniform int skyHDR;
uniform float skyMaxLod;
uniform int fogFromSky;

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

vec3 aces(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
// Sky radiance in direction d, converted to display colour exactly like the sky shader
vec3 skyColor(vec3 d, float lod) {
    vec3 c = textureLod(skyTex, skyRot * d, lod).rgb * skyExposure;
    if (skyHDR == 1) c = aces(c);
    return pow(clamp(c, 0.0, 1.0), vec3(1.0 / 2.2));
}

vec3 applyFog(vec3 color, float dist, vec3 rayDir) {
    if (underwater == 1) {
        // Light is absorbed quickly under water: strong blue-green fog
        float f = 1.0 - exp(-dist * 0.035);
        return mix(color * vec3(0.6, 0.9, 1.0), uwColor, f);
    }
    float f = 1.0 - exp(-pow(fogDensity * dist, 2.0));
    vec3 fc = fogColor;
    if (hasSky == 1 && fogFromSky == 1) {
        // Fog takes the colour of the sky just above the horizon behind the object,
        // so distant terrain melts into the panorama instead of ending in a hard line
        vec3 d = normalize(vec3(rayDir.x, 0.04 + max(rayDir.y, 0.0) * 0.5, rayDir.z));
        fc = skyColor(d, max(skyMaxLod - 4.0, 0.0));
    }
    return mix(color, fc, clamp(f, 0.0, 1.0));
}

// Animated caustic web (light focused by the waves onto the sea floor)
float caustics(vec2 p, float t) {
    vec2 i = p * 0.35;
    vec2 q = i;
    float c = 1.0;
    const float inten = 0.005;
    for (int n = 0; n < 4; n++) {
        float tt = t * 0.6 * (1.0 - (3.5 / float(n + 1)));
        q = i + vec2(cos(tt - q.x) + sin(tt + q.y), sin(tt - q.y) + cos(tt + q.x));
        c += 1.0 / length(vec2(i.x / (sin(q.x + tt) / inten), i.y / (cos(q.y + tt) / inten)));
    }
    c /= 4.0;
    c = 1.17 - pow(c, 1.4);
    return clamp(pow(abs(c), 8.0), 0.0, 1.0);
}

// --- Material sampling ---
// Top-down projection with a whiteout-blended normal map
void sampleTop(int layer, vec2 uv, vec3 N, out vec3 albedo, out vec3 n) {
    albedo = texture(matAlbedo, vec3(uv, layer)).rgb;
    vec3 t = texture(matNormal, vec3(uv, layer)).xyz * 2.0 - 1.0;
    n = normalize(vec3(t.x + N.x, abs(t.z) * N.y, t.y + N.z));
}
// Triplanar projection for cliffs (no stretching on steep faces)
void sampleTriplanar(int layer, vec3 p, vec3 N, out vec3 albedo, out vec3 n) {
    vec3 w = pow(abs(N), vec3(4.0));
    w /= (w.x + w.y + w.z);
    albedo = texture(matAlbedo, vec3(p.zy, layer)).rgb * w.x
           + texture(matAlbedo, vec3(p.xz, layer)).rgb * w.y
           + texture(matAlbedo, vec3(p.xy, layer)).rgb * w.z;
    vec3 tx = texture(matNormal, vec3(p.zy, layer)).xyz * 2.0 - 1.0;
    vec3 ty = texture(matNormal, vec3(p.xz, layer)).xyz * 2.0 - 1.0;
    vec3 tz = texture(matNormal, vec3(p.xy, layer)).xyz * 2.0 - 1.0;
    tx = vec3(tx.xy + N.zy, abs(tx.z) * N.x);
    ty = vec3(ty.xy + N.xz, abs(ty.z) * N.y);
    tz = vec3(tz.xy + N.xy, abs(tz.z) * N.z);
    n = normalize(tx.zyx * w.x + ty.xzy * w.y + tz.xyz * w.z);
}

vec3 terrainColor(vec3 N, vec3 viewDir, float dist, vec3 light) {
    vec3 P = FragPos;
    float slope = 1.0 - N.y;
    float macro = fbm(P.xz * 0.015);
    float mid = fbm(P.xz * 0.12);

    // Layer weights
    float wRock = smoothstep(rockSlope, rockSlope + 0.12, slope + (mid - 0.5) * 0.12);
    float wSand = 1.0 - smoothstep(waterY + beachWidth * 0.3, waterY + beachWidth + (mid - 0.5) * beachWidth, P.y);
    wSand *= 1.0 - wRock * 0.8;
    float wSnow = smoothstep(snowLine, snowLine + snowBlend, P.y + (mid - 0.5) * snowBlend - slope * snowBlend * 1.2);
    wSnow *= 1.0 - smoothstep(0.35, 0.7, slope);
    float wGrass2 = smoothstep(0.52, 0.72, macro) * 0.75;   // patches of drier, leafy ground

    vec3 albedo, n;
    if (hasMaterials == 1) {
        vec2 uvG = P.xz * (0.16 * texScale);
        vec2 uvS = P.xz * (0.2 * texScale);
        vec3 aG, nG, aG2, nG2, aR, nR, aS, nS, aSn, nSn;
        if (customGrass == 1) { aG = texture(texture1, uvG).rgb; nG = N; }
        else sampleTop(0, uvG, N, aG, nG);
        // A second, larger-scale sample breaks up visible tiling in the distance
        vec3 aFar = texture(matAlbedo, vec3(uvG * 0.13 + 0.37, 0)).rgb;
        aG = mix(aG, aG * aFar * 2.2, 0.35 + 0.3 * smoothstep(20.0, 120.0, dist));
        sampleTop(1, uvG * 0.8, N, aG2, nG2);
        albedo = mix(aG, aG2, wGrass2) * grassTint;
        n = normalize(mix(nG, nG2, wGrass2));
        if (wSand > 0.001) {
            sampleTop(3, uvS, N, aS, nS);
            albedo = mix(albedo, aS * sandTint, wSand); n = normalize(mix(n, nS, wSand));
        }
        if (wRock > 0.001) {
            sampleTriplanar(2, P * (0.09 * texScale), N, aR, nR);
            albedo = mix(albedo, aR * rockTint, wRock); n = normalize(mix(n, nR, wRock));
        }
        if (wSnow > 0.001) {
            sampleTop(4, uvS * 0.6, N, aSn, nSn);
            albedo = mix(albedo, aSn * snowTint, wSnow); n = normalize(mix(n, nSn, wSnow));
        }
    } else {
        // Fallback: flat colours when the material textures are missing
        vec3 grass = (customGrass == 1 ? texture(texture1, TexCoords).rgb : vec3(0.30, 0.45, 0.18)) * grassTint;
        albedo = mix(grass, vec3(0.8, 0.72, 0.5) * sandTint, wSand);
        albedo = mix(albedo, vec3(0.42, 0.38, 0.35) * rockTint * (0.7 + 0.6 * mid), wRock);
        albedo = mix(albedo, vec3(0.95, 0.97, 1.0) * snowTint, wSnow);
        n = N;
    }
    albedo *= 0.85 + 0.3 * macro;
    // Wet sand darkens near the waterline; the sea floor takes on the water's colour with depth
    albedo *= mix(0.6, 1.0, smoothstep(waterY - 0.3, waterY + 0.5, P.y));
    float depthBelow = waterY - P.y;
    if (depthBelow > 0.0) albedo = mix(albedo, albedo * waterShallow * 1.6, clamp(depthBelow / 8.0, 0.0, 0.8));

    // Lighting: sky ambient + soft wrapped sun
    vec3 skyAmb = vec3(0.50, 0.62, 0.80), groundAmb = vec3(0.28, 0.24, 0.20);
    if (hasSky == 1) {
        skyAmb = skyColor(normalize(n + vec3(0.0, 0.6, 0.0)), max(skyMaxLod - 1.0, 0.0)) * 0.9;
        groundAmb = skyAmb * vec3(0.55, 0.48, 0.40);
    }
    vec3 amb = mix(groundAmb, skyAmb, n.y * 0.5 + 0.5) * 0.55;
    float diff = max(dot(n, light), 0.0);
    float wrap = clamp((dot(n, light) + 0.25) / 1.25, 0.0, 1.0);
    // Shade from the smooth mesh normal too so the big shapes stay readable
    float macroShade = mix(0.75, 1.0, clamp(dot(N, light) * 0.5 + 0.5, 0.0, 1.0));
    vec3 lit = (amb + lightColor * (0.55 * diff + 0.45 * wrap * wrap) * macroShade) * albedo;
    float spec = pow(max(dot(viewDir, reflect(-light, n)), 0.0), 32.0);
    lit += lightColor * spec * (0.25 * wSnow + 0.12 * wSand * (1.0 - smoothstep(waterY, waterY + 0.6, P.y)));

    if (depthBelow > 0.0) {
        float c = caustics(P.xz, time) * exp(-depthBelow * 0.12) * max(dot(N, light), 0.0);
        lit += lightColor * c * 1.3;
    }
    return lit;
}

void main() {
    vec3 light = normalize(-lightDir);
    vec3 viewVec = viewPos - FragPos;
    float dist = length(viewVec);
    vec3 viewDir = viewVec / dist;
    vec3 norm = normalize(Normal);

    if (shadeMode == 1) {
        FragColor = vec4(applyFog(terrainColor(norm, viewDir, dist, light), dist, -viewDir), 1.0);
        return;
    }

    // ---- Plain shading (models) ----
    float diff = max(dot(norm, light), 0.0);
    vec3 reflectDir = reflect(-light, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 baseColor = (useSolidColor == 1) ? solidColor : texture(texture1, TexCoords).rgb;
    vec3 color = (0.25 + diff * lightColor) * baseColor + 0.25 * spec * lightColor;
    FragColor = vec4(applyFog(color, dist, -viewDir), 1.0);
}
