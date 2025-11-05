#version 330 core
out vec4 FragColor;
in vec3 vDir;

uniform samplerCube skybox;
uniform bool hasSkybox;

// Cloud controls
uniform float time;            // seconds
uniform bool cloudEnabled;
uniform float cloudSpeed;
uniform float cloudScale;
uniform float cloudOpacity;

// --- Utility noise functions ---
float hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5,183.3)));
    return fract(sin(p.x + p.y) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash(i + vec2(0.0,0.0));
    float b = hash(i + vec2(1.0,0.0));
    float c = hash(i + vec2(0.0,1.0));
    float d = hash(i + vec2(1.0,1.0));
    vec2 u = f*f*(3.0-2.0*f);
    return mix(a,b,u.x) + (c-a)*u.y*(1.0-u.x) + (d-b)*u.x*u.y;
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 5; ++i) {
        v += a * noise(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

// --- Main ---
void main() {
    vec3 dir = normalize(vDir);

    // Base sky color
    vec3 baseColor;
    if (!hasSkybox) {
        // Procedural gradient fallback
        float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 skyTop = vec3(0.4, 0.6, 1.0);
        vec3 skyBottom = vec3(0.85, 0.95, 1.0);
        baseColor = mix(skyBottom, skyTop, pow(t, 1.3));
    } else {
        // Smoothly sample cubemap and slightly blur for seamless look
        // Higher LOD = smoother, lower LOD = sharper
        baseColor = textureLod(skybox, dir, 1.5).rgb;

        // Optional: gradient tint to reduce edge artifacts near horizon
        float horizon = smoothstep(-0.1, 0.3, dir.y);
        vec3 tint = vec3(0.95, 0.9, 1.0);
        baseColor = mix(baseColor * tint, baseColor, horizon);
    }

    vec3 color = baseColor;

    // --- Clouds overlay ---
    if (cloudEnabled) {
        float lon = atan(-dir.z, dir.x);
        float lat = asin(clamp(dir.y, -1.0, 1.0));
        vec2 uv = vec2((lon / (2.0 * 3.141592653589793)) + 0.5,
                       0.5 + lat / 3.141592653589793);

        vec2 p = uv * cloudScale;
        p.x += time * cloudSpeed;

        float n = fbm(p * 6.0);
        float clouds = smoothstep(0.55, 0.8, n);
        float feather = smoothstep(0.4, 0.55, n) * (1.0 - smoothstep(0.8, 0.95, n));
        float cloudMask = mix(feather, clouds, 0.6);

        vec3 cloudColor = vec3(1.0);
        color = mix(color, cloudColor, cloudMask * cloudOpacity);
    }

    FragColor = vec4(color, 1.0);
}
