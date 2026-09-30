#pragma once

// Rendering quality and post-processing settings (edited live in the GUI)
struct GraphicsSettings {
    // Resolution: the 3D scene is rendered at renderScale x window size and upscaled.
    // With autoResolution the scale adapts every frame to hold targetFps.
    bool  autoResolution = true;
    float targetFps = 60.0f;
    float renderScale = 1.0f;      // manual scale (used when autoResolution is off)
    float minScale = 0.5f;
    float maxScale = 1.0f;

    // Terrain detail: distance (m) where terrain chunks start dropping resolution
    float terrainLodDistance = 110.0f;

    // Lighting
    bool  shadows = true;
    float shadowStrength = 0.85f;

    // Atmosphere
    float fogHeightDensity = 0.0035f;  // extra haze in valleys / over the sea
    float fogHeightFalloff = 0.05f;
    float sunGlow = 0.22f;             // haze lit up around the sun

    // Post-processing
    bool  bloom = true;
    float bloomIntensity = 0.25f;
    float bloomThreshold = 0.9f;
    bool  godRays = true;
    float godRayIntensity = 0.35f;
    bool  fxaa = true;
    float sharpen = 0.35f;
    bool  vignette = true;
    float vignetteStrength = 0.3f;
    float exposure = 1.0f;
    float contrast = 1.06f;
    float saturation = 1.08f;
    float warmth = 0.03f;
    bool  underwaterWobble = true;

    // 0 low, 1 medium, 2 high (applies a bundle of the settings above)
    static GraphicsSettings preset(int level) {
        GraphicsSettings g;
        if (level == 0) {
            g.targetFps = 60.0f; g.minScale = 0.5f; g.maxScale = 0.85f; g.terrainLodDistance = 70.0f;
            g.godRays = false; g.bloom = true; g.sharpen = 0.5f;
        } else if (level == 2) {
            g.minScale = 0.7f; g.maxScale = 1.0f; g.terrainLodDistance = 170.0f;
        }
        return g;
    }
};
