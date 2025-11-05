#include "gui.h"
#include "../Nut.h"

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

void GUI::render() {
    if (!initialized_) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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

    if (ImGui::Button("Regenerate Terrain")) {
        engine_->regenerateTerrain();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
