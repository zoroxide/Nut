#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 WaveNormal;
in float Crest;
in float Depth;

uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float time;
uniform float waterOpacity;
uniform vec3 waterShallow;
uniform vec3 waterDeep;
uniform vec3 fogColor;
uniform float fogDensity;
uniform int underwater;
uniform vec3 uwColor;
uniform float choppiness;

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
vec3 skyColor(vec3 d, float lod) {
    vec3 c = textureLod(skyTex, skyRot * d, lod).rgb * skyExposure;
    if (skyHDR == 1) c = aces(c);
    return pow(clamp(c, 0.0, 1.0), vec3(1.0 / 2.2));
}
vec3 skyOrDefault(vec3 d, float lod) {
    if (hasSky == 1) return skyColor(d, lod);
    return mix(fogColor, vec3(0.25, 0.45, 0.80), smoothstep(0.0, 0.6, d.y));
}

void main() {
    vec3 light = normalize(-lightDir);
    vec3 viewVec = viewPos - FragPos;
    float dist = length(viewVec);
    vec3 v = viewVec / dist;
    vec2 p = FragPos.xz;

    // Fine ripples on top of the geometric waves (fade with distance to avoid shimmer)
    vec2 w1 = vec2(fbm(p * 0.30 + time * 0.12), fbm(p * 0.30 - time * 0.10 + 17.0));
    vec2 w2 = vec2(fbm(p * 1.20 - time * 0.30 + 3.0), fbm(p * 1.20 + time * 0.26 + 9.0));
    vec2 w3 = vec2(vnoise(p * 4.0 + time * vec2(0.9, 0.4)), vnoise(p * 4.0 - time * vec2(0.5, 0.8) + 7.0));
    vec2 d = ((w1 - 0.5) * 0.45 + (w2 - 0.5) * 0.25 + (w3 - 0.5) * 0.18 * (1.0 - smoothstep(10.0, 60.0, dist)))
             * (1.0 - smoothstep(60.0, 400.0, dist));
    vec3 n = normalize(WaveNormal + vec3(d.x, 0.0, d.y));
    float choppyFoam = 0.6 * smoothstep(0.2, 1.0, choppiness);

    float shore = clamp(Depth, 0.0, 100.0);

    if (underwater == 1) {
        // ---- Seen from below: Snell's window shows the sky, outside it the surface mirrors the deep ----
        vec3 nb = -n;                              // surface normal facing down, towards the camera
        float cosT = dot(v, nb);                   // 1 = looking straight up at the surface
        // Inside ~48.6 deg (critical angle) light from the sky gets through; outside, total internal reflection
        float window = smoothstep(0.62, 0.72, cosT);
        vec3 refr = refract(-v, nb, 1.33);
        vec3 through = skyOrDefault(dot(refr, refr) > 0.0 ? normalize(refr) : vec3(0.0, 1.0, 0.0), 2.0) * 1.1;
        vec3 col = mix(uwColor * 0.8, through, window);
        col += lightColor * pow(max(dot(-v, light), 0.0), 40.0) * window;
        float f = 1.0 - exp(-dist * 0.035);
        FragColor = vec4(mix(col, uwColor, f), 0.97);
        return;
    }

    // ---- Seen from above ----
    float F = 0.02 + 0.98 * pow(1.0 - max(dot(n, v), 0.0), 5.0);
    vec3 refl = skyOrDefault(reflect(-v, n), 1.0);

    vec3 body = mix(waterShallow, waterDeep, smoothstep(0.0, 9.0, shore));
    float diff = max(dot(n, light), 0.0);
    body *= 0.45 + 0.55 * diff * length(lightColor) * 0.6 + 0.3;
    // Light shining through the back of wave crests
    float sss = pow(max(dot(v, -light), 0.0), 3.0) * max(Crest, 0.0);
    body += waterShallow * lightColor * sss * 0.6;

    vec3 col = mix(body, refl, F);
    vec3 h = normalize(light + v);
    // Sun glint: a tight, bright highlight that breaks into sparkles on the ripples
    col += lightColor * pow(max(dot(n, h), 0.0), 600.0) * 1.6;

    // Foam: breaking at the shoreline and on the sharpest crests
    // Bands of surf rolling towards the beach, broken up by noise, thickest right at the waterline
    float foamNoise = fbm(p * 2.2 + time * vec2(0.25, 0.1));
    float surf = smoothstep(0.55, 0.95, sin(shore * 5.0 - time * 1.6 + foamNoise * 3.0) * 0.5 + 0.5);
    float edgeFoam = 1.0 - smoothstep(0.0, 0.25 + 0.2 * foamNoise, shore);
    float shoreFoam = max(edgeFoam, surf * (1.0 - smoothstep(0.2, 1.6, shore)) * 0.8) * smoothstep(0.25, 0.6, foamNoise);
    // Whitecaps: only on the highest, sharpest crests, as thin streaks
    float streak = fbm(p * vec2(4.0, 1.2) + time * 0.4);
    float crestFoam = smoothstep(0.72, 1.0, Crest) * smoothstep(0.55, 0.8, streak) * choppyFoam;
    float foam = clamp(shoreFoam + crestFoam, 0.0, 1.0);
    col = mix(col, vec3(0.95) * (0.6 + 0.4 * lightColor), foam * 0.9);

    float alpha = clamp(waterOpacity * smoothstep(0.0, 0.6, shore) + F * 0.5 + foam * 0.6, 0.0, 1.0);
    if (shore > 50.0) alpha = 1.0;   // open ocean beyond the island is opaque

    // Atmospheric fog (same as terrain)
    float fogF = 1.0 - exp(-pow(fogDensity * dist, 2.0));
    vec3 fc = fogColor;
    if (hasSky == 1 && fogFromSky == 1)
        fc = skyColor(normalize(vec3(-v.x, 0.04 + max(-v.y, 0.0) * 0.5, -v.z)), max(skyMaxLod - 4.0, 0.0));
    col = mix(col, fc, clamp(fogF, 0.0, 1.0));
    alpha = mix(alpha, 1.0, fogF);
    FragColor = vec4(col, alpha);
}
