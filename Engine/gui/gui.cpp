#include "gui.h"
#include "../Engine.h"

#include "../libs/imgui/imgui.h"
#include "../libs/imgui/backends/imgui_impl_glfw.h"
#include "../libs/imgui/backends/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <cstring>

GUI::GUI(Engine* engine) : engine_(engine), window_(nullptr), initialized_(false) {}

GUI::~GUI() {
    if (initialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

bool GUI::init(GLFWwindow* window) {
    if (initialized_) return true;
    window_ = window;

    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Style
    ImGui::StyleColorsDark();

    // Backends
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        std::cerr << "Failed to init ImGui GLFW backend" << std::endl; return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "Failed to init ImGui OpenGL3 backend" << std::endl; return false;
    }

    initialized_ = true;
    return true;
}

void GUI::drawTerrainPanel() {
    TerrainParams& P = engine_->terrainParams();
    static bool autoRegen = false;
    static int preset = 0;
    bool changed = false, force = false;

    if (!ImGui::CollapsingHeader("Procedural Terrain", ImGuiTreeNodeFlags_DefaultOpen)) return;

    int nPresets = 0;
    const char* const* names = TerrainParams::presetNames(nPresets);
    ImGui::Combo("Preset", &preset, names, nPresets);
    ImGui::SameLine();
    if (ImGui::Button("Apply")) {
        int seed = P.seed;
        P = TerrainParams::preset(preset);
        P.seed = seed;
        force = true;
    }

    ImGui::InputInt("Seed", &P.seed);
    ImGui::SameLine();
    if (ImGui::Button("Random")) { P.seed = (int)(ImGui::GetTime() * 1000.0) ^ (rand() & 0xffff); force = true; }

    if (ImGui::TreeNodeEx("Shape", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::SliderFloat("Frequency", &P.frequency, 0.001f, 0.02f, "%.4f", ImGuiSliderFlags_Logarithmic);
        changed |= ImGui::SliderInt("Octaves", &P.octaves, 1, 10);
        changed |= ImGui::SliderFloat("Persistence", &P.persistence, 0.2f, 0.8f);
        changed |= ImGui::SliderFloat("Lacunarity", &P.lacunarity, 1.5f, 3.5f);
        changed |= ImGui::SliderFloat("Ridged Mountains", &P.ridgeAmount, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Domain Warp", &P.warpStrength, 0.0f, 200.0f);
        changed |= ImGui::SliderFloat("Height Curve", &P.heightPower, 0.5f, 3.5f);
        changed |= ImGui::SliderFloat("Terracing", &P.terraceStrength, 0.0f, 1.0f);
        changed |= ImGui::SliderInt("Terrace Steps", &P.terraceSteps, 2, 20);
        changed |= ImGui::SliderFloat("Island Falloff", &P.islandStrength, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Island Radius", &P.islandRadius, 0.1f, 0.95f);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Erosion")) {
        changed |= ImGui::SliderInt("Hydraulic Droplets", &P.erosionIterations, 0, 300000);
        changed |= ImGui::SliderFloat("Erode Strength", &P.erosionStrength, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Deposit Strength", &P.depositStrength, 0.0f, 1.0f);
        changed |= ImGui::SliderInt("Thermal Passes", &P.thermalIterations, 0, 30);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Water")) {
        changed |= ImGui::Checkbox("Enable Water", &P.waterEnabled);
        changed |= ImGui::SliderFloat("Water Level", &P.waterLevel, 0.0f, 0.9f);
        ImGui::SliderFloat("Water Opacity", &P.waterOpacity, 0.2f, 1.0f);
        ImGui::ColorEdit3("Shallow Color", &P.waterShallow.x);
        ImGui::ColorEdit3("Deep Color", &P.waterDeep.x);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Materials")) {
        ImGui::SliderFloat("Beach Width", &P.beachWidth, 0.0f, 0.15f);
        ImGui::SliderFloat("Rock Slope", &P.rockSlope, 0.05f, 0.9f);
        ImGui::SliderFloat("Snow Line", &P.snowLine, 0.2f, 1.2f);
        ImGui::SliderFloat("Snow Blend", &P.snowBlend, 0.01f, 0.4f);
        ImGui::ColorEdit3("Grass Tint", &P.grassTint.x);
        ImGui::ColorEdit3("Sand", &P.sandColor.x);
        ImGui::ColorEdit3("Rock", &P.rockColor.x);
        ImGui::ColorEdit3("Snow", &P.snowColor.x);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Atmosphere")) {
        ImGui::SliderFloat("Fog Density", &P.fogDensity, 0.0f, 0.01f, "%.4f");
        ImGui::ColorEdit3("Fog Color", &P.fogColor.x);
        ImGui::TreePop();
    }

    ImGui::Checkbox("Auto Regenerate (slow with erosion)", &autoRegen);
    // While dragging a slider just remember that something changed; rebuild on release
    static bool pending = false;
    if (changed && autoRegen) pending = true;
    if (!autoRegen) pending = false;
    bool rebuild = force || (pending && !ImGui::IsMouseDown(0));
    if (ImGui::Button("Regenerate Terrain")) rebuild = true;
    if (rebuild) { pending = false; engine_->regenerateTerrain(); }
}

void GUI::render() {
    if (!initialized_) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // HUD: top-left coin counter
    {
        ImGui::SetNextWindowPos(ImVec2(10,10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav);
    ImGui::Text("Coins: %d", engine_->getCoinsCollected());
        ImGui::End();
    }

    ImGui::Begin("Engine Controls");

    // Performance
    {
        ImGuiIO& io = ImGui::GetIO();
        float fps = io.Framerate;
        float ms = fps > 0.0f ? 1000.0f / fps : 0.0f;
        ImGui::Text("FPS: %.1f (%.3f ms)", fps, ms);

        int vsMode = engine_->getVsyncEnabled() ? 1 : 0;
        int prev = vsMode;
        if (ImGui::RadioButton("VSync On", vsMode == 1)) vsMode = 1;
        ImGui::SameLine();
        if (ImGui::RadioButton("VSync Off", vsMode == 0)) vsMode = 0;
        if (vsMode != prev) {
            engine_->vsync(vsMode == 1);
        }
    }

    ImGui::Separator();

    // Skybox (folder with faces OR single equirectangular .png)
    char pbuf[512];
    std::string currentP = engine_->getPanoramaPath();
    strncpy(pbuf, currentP.c_str(), sizeof(pbuf)); pbuf[sizeof(pbuf)-1] = '\0';

    if (ImGui::InputText("Skybox Path (folder: right/left/top/bottom/front/back .png|.bmp OR single equirectangular .png|.bmp)", pbuf, sizeof(pbuf))) {
        engine_->setPanoramaPath(std::string(pbuf));
    }

    if (ImGui::Button("Load Skybox")) {
        engine_->panorama(engine_->getPanoramaPath());
    }

    ImGui::Separator();

    // Terrain texture
    char tbuf[512];
    std::string currentT = engine_->getTerrainTexturePath();
    strncpy(tbuf, currentT.c_str(), sizeof(tbuf)); tbuf[sizeof(tbuf)-1] = '\0';
    if (ImGui::InputText("Terrain Texture Path", tbuf, sizeof(tbuf))) {
        engine_->setTerrainTexturePath(std::string(tbuf));
    }
    if (ImGui::Button("Load Terrain Texture and Tree")) {
        engine_->load_terrain_using_texture(engine_->getTerrainTexturePath(), "assets/Tree1/Tree1.obj");
    }

    ImGui::Separator();

    // Houses controls
    static char hpath[512] = "assets/house.obj";
    ImGui::InputText("House OBJ Path", hpath, sizeof(hpath));
    static float hpos[3] = {0.0f, 0.0f, 0.0f};
    static float hscale = 1.0f;
    ImGui::InputFloat3("House Position (x,y,z)", hpos);
    ImGui::InputFloat("House Uniform Scale", &hscale);
    if (ImGui::Button("Add House")) {
        engine_->add_house(std::string(hpath), glm::vec3(hpos[0], hpos[1], hpos[2]), glm::vec3(hscale));
    }
    ImGui::SameLine();
    if (ImGui::Button("Add House At Camera")) {
        // Use current camera position with slight offset above terrain
        // As GUI doesn't have direct camera getters, we place at origin scale; user can adjust.
        engine_->add_house(std::string(hpath), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(hscale));
    }

    // Constants
    int ts = engine_->getTerrainSize();
    if (ImGui::InputInt("Terrain Size", &ts)) {
        if (ts < 2) ts = 2;
        engine_->setTerrainSize(ts);
    }

    float sc = engine_->getTerrainScale();
    if (ImGui::InputFloat("Terrain Scale", &sc)) engine_->setTerrainScale(sc);

    float hs = engine_->getHeightScale();
    if (ImGui::InputFloat("Height Scale", &hs)) engine_->setHeightScale(hs);

    float tt = engine_->getTextureTile();
    if (ImGui::InputFloat("Texture Tile", &tt)) engine_->setTextureTile(tt);

    // Cloud controls
    bool ce = engine_->getCloudEnabled();
    if (ImGui::Checkbox("Enable Clouds", &ce)) engine_->setCloudEnabled(ce);

    float cs = engine_->getCloudSpeed();
    if (ImGui::SliderFloat("Cloud Speed", &cs, 0.0f, 0.5f)) engine_->setCloudSpeed(cs);

    float csc = engine_->getCloudScale();
    if (ImGui::SliderFloat("Cloud Scale", &csc, 0.2f, 4.0f)) engine_->setCloudScale(csc);

    float cop = engine_->getCloudOpacity();
    if (ImGui::SliderFloat("Cloud Opacity", &cop, 0.0f, 1.0f)) engine_->setCloudOpacity(cop);

    drawTerrainPanel();

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
