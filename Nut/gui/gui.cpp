#include "gui.h"
#include "../Nut.h"

// ImGui includes (user must have ImGui and the backends available)
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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

    // Panorama
    char pbuf[512];
    std::string currentP = engine_->getPanoramaPath();
    strncpy(pbuf, currentP.c_str(), sizeof(pbuf)); pbuf[sizeof(pbuf)-1] = '\0';
    if (ImGui::InputText("Panorama Path", pbuf, sizeof(pbuf))) {
        engine_->setPanoramaPath(std::string(pbuf));
    }
    if (ImGui::Button("Load Panorama")) {
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
    if (ImGui::Button("Load Terrain Texture")) {
        engine_->load_terrain_using_texture(engine_->getTerrainTexturePath());
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

    if (ImGui::Button("Regenerate Terrain")) {
        engine_->regenerateTerrain();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
