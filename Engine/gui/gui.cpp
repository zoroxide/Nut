#include "gui.h"
#include "../Engine.h"

#include "../libs/imgui/imgui.h"
#include "../libs/imgui/backends/imgui_impl_glfw.h"
#include "../libs/imgui/backends/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <vector>

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

// Lists loadable skies: equirect images in assets/panoramas + assets/skybox, and cube-face folders
static std::vector<std::string> scanSkies() {
    namespace fs = std::filesystem;
    std::vector<std::string> out;
    std::error_code ec;
    for (const char* dir : {"assets/panoramas", "assets/skybox"}) {
        for (auto it = fs::directory_iterator(dir, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
            const fs::path& p = it->path();
            std::string ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (it->is_directory(ec)) {
                if (fs::exists(p / "right.png", ec) || fs::exists(p / "right.bmp", ec) || fs::exists(p / "right.jpg", ec))
                    out.push_back(p.string());
            } else if (ext == ".hdr" || ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
                // Skip helper images that are not panoramas
                std::string name = p.filename().string();
                if (name.find("UV") != std::string::npos || name == "pz.png") continue;
                out.push_back(p.string());
            }
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

void GUI::drawSkyPanel() {
    if (!ImGui::CollapsingHeader("Sky / Panorama", ImGuiTreeNodeFlags_DefaultOpen)) return;

    static std::vector<std::string> skies = scanSkies();
    static int selected = -1;
    const std::string& current = engine_->getPanoramaPath();
    if (selected < 0) {
        for (size_t i = 0; i < skies.size(); ++i) if (skies[i] == current) selected = (int)i;
    }

    auto label = [](const std::string& p) { return std::filesystem::path(p).filename().string(); };
    std::string preview = selected >= 0 && selected < (int)skies.size() ? label(skies[selected])
                        : (current.empty() ? std::string("Procedural sky") : label(current));
    if (ImGui::BeginCombo("Sky", preview.c_str())) {
        if (ImGui::Selectable("Procedural sky", current.empty())) { engine_->panorama(""); selected = -1; }
        for (size_t i = 0; i < skies.size(); ++i) {
            bool isSel = (int)i == selected;
            if (ImGui::Selectable(label(skies[i]).c_str(), isSel)) {
                if (engine_->panorama(skies[i])) selected = (int)i;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", skies[i].c_str());
            if (isSel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Rescan")) { skies = scanSkies(); selected = -1; }

    // Manual path (folder with right/left/top/bottom/front/back, or a single equirect image)
    static char pbuf[512] = "";
    ImGui::InputTextWithHint("##skypath", "custom path (.hdr/.png/.jpg or cube-face folder)", pbuf, sizeof(pbuf));
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (engine_->panorama(pbuf)) selected = -1;
    }

    Skybox& sky = engine_->sky();
    if (sky.hasCubemap()) {
        ImGui::TextDisabled("%s panorama%s", sky.isHDR() ? "HDR" : "LDR",
                            sky.analysis().hasSun ? " - sun detected" : "");
        ImGui::SliderFloat("Exposure", &sky.exposure, 0.02f, 8.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::SameLine();
        if (ImGui::SmallButton("Auto")) sky.exposure = sky.analysis().autoExposure;
        ImGui::SliderFloat("Rotation", &sky.rotationDeg, 0.0f, 360.0f, "%.0f deg");
        ImGui::SliderFloat("Blur", &sky.blur, 0.0f, 6.0f);
        ImGui::Checkbox("Sun & light from sky", &engine_->sunFromSky());
        ImGui::SameLine();
        ImGui::Checkbox("Fog from sky", &engine_->fogFromSky());
    }
    ImGui::Checkbox("Fill below horizon", &sky.horizonFill);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hide the panorama's floor behind the horizon haze");

    bool ce = engine_->getCloudEnabled();
    if (ImGui::Checkbox("Procedural Clouds", &ce)) engine_->setCloudEnabled(ce);
    if (ce) {
        float cs = engine_->getCloudSpeed();
        if (ImGui::SliderFloat("Cloud Speed", &cs, 0.0f, 0.5f)) engine_->setCloudSpeed(cs);
        float csc = engine_->getCloudScale();
        if (ImGui::SliderFloat("Cloud Scale", &csc, 0.2f, 4.0f)) engine_->setCloudScale(csc);
        float cop = engine_->getCloudOpacity();
        if (ImGui::SliderFloat("Cloud Opacity", &cop, 0.0f, 1.0f)) engine_->setCloudOpacity(cop);
    }
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
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Island", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::SliderFloat("Island Strength", &P.islandStrength, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Island Size", &P.islandRadius, 0.1f, 0.6f);
        changed |= ImGui::SliderFloat("Coastline Roughness", &P.coastNoise, 0.0f, 1.5f);
        changed |= ImGui::SliderFloat("Ocean Depth", &P.seaDepth, 0.05f, 0.5f);
        changed |= ImGui::Checkbox("No Lakes (fill inland basins)", &P.fillLakes);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Erosion")) {
        changed |= ImGui::SliderInt("Hydraulic Droplets", &P.erosionIterations, 0, 300000);
        changed |= ImGui::SliderFloat("Erode Strength", &P.erosionStrength, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Deposit Strength", &P.depositStrength, 0.0f, 1.0f);
        changed |= ImGui::SliderInt("Thermal Passes", &P.thermalIterations, 0, 30);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Ocean")) {
        changed |= ImGui::Checkbox("Enable Ocean", &P.waterEnabled);
        changed |= ImGui::SliderFloat("Sea Level", &P.waterLevel, 0.0f, 0.9f);
        // Waves update live (no regeneration needed)
        ImGui::SliderFloat("Wave Height (m)", &P.waveHeight, 0.0f, 3.0f);
        ImGui::SliderFloat("Wave Length (m)", &P.waveLength, 8.0f, 120.0f);
        ImGui::SliderFloat("Choppiness", &P.choppiness, 0.0f, 1.0f);
        ImGui::SliderFloat("Wind Direction", &P.windAngle, 0.0f, 360.0f, "%.0f deg");
        ImGui::SliderFloat("Wave Speed", &P.waveSpeed, 0.0f, 3.0f);
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
        ImGui::SliderFloat("Texture Scale", &P.textureScale, 0.3f, 4.0f);
        ImGui::ColorEdit3("Grass Tint", &P.grassTint.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
        ImGui::ColorEdit3("Sand Tint", &P.sandTint.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
        ImGui::ColorEdit3("Rock Tint", &P.rockTint.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
        ImGui::ColorEdit3("Snow Tint", &P.snowTint.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
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

    // HUD: swimming status and oxygen (only shown in the water)
    if (engine_->isSwimming() || engine_->getOxygen() < 0.999f) {
        ImGui::SetNextWindowPos(ImVec2(10,10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav);
        if (engine_->isSwimming()) {
            ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), engine_->isUnderwater() ? "Diving" : "Swimming");
            ImGui::TextDisabled("C/Ctrl dive - Space up - look down + W to dive");
        }
        float o2 = engine_->getOxygen();
        if (o2 < 0.999f || engine_->isUnderwater()) {
            ImVec4 col = o2 > 0.3f ? ImVec4(0.35f, 0.75f, 1.0f, 1.0f) : ImVec4(1.0f, 0.35f, 0.3f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
            ImGui::ProgressBar(o2, ImVec2(160, 0), "Oxygen");
            ImGui::PopStyleColor();
        }
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

    drawSkyPanel();

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

    drawTerrainPanel();

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
