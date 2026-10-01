#include "Engine/Engine.h"
#include <iostream>
#include <fstream>
#include <cstring>

int main(int argc, char** argv) {
    bool smoke = argc > 1 && std::strcmp(argv[1], "--smoke-test") == 0;
    bool windowed = smoke || (argc > 1 && std::strcmp(argv[1], "--windowed") == 0);
    Engine engine;

    // Initialize the engine (fullscreen by default). If you want windowed, pass false.
    if (!engine.init(!windowed)) {
        std::cerr << "Failed to initialize engine\n";
        return -1;
    }

    // Ground materials (grass/rock/sand/snow) load automatically from assets/textures/terrain.
    // To force your own grass texture instead:
    // engine.load_terrain_using_texture("assets/textures/grass.png");

    // Load panorama (optional). HDR panoramas also drive the sun direction, colour and fog.
    if (!engine.panorama("assets/panoramas/kloofendal_48d_partly_cloudy_puresky_4k.hdr") &&
        !engine.panorama("assets/skybox/sky_17_2k.png")) {
        std::cerr << "Failed to load panorama texture\n";
    }

    // Toggle vsync if desired
    engine.vsync(true);

    // A reproducible render/collision check, without entering the interactive loop.
    if (smoke) {
        if (!engine.village().active()) return 2;
        glm::vec3 center = engine.village().center();
        glm::vec3 door = center + glm::vec3(-30,1.7f,-13);
        glm::vec3 wall = center + glm::vec3(-27,1.7f,-13);
        glm::vec3 originalDoor = door, originalWall = wall;
        engine.village().collide(door);
        engine.village().collide(wall);
        if (glm::length(door-originalDoor)>0.01f || glm::length(wall-originalWall)<0.1f) return 3;
        while (glGetError()!=GL_NO_ERROR) {}
        for (int i=0;i<3;++i) engine.renderFrame(0,1280,720);
        glFinish();
        GLenum error=glGetError();
        std::vector<unsigned char> pixels(1280*720*3);
        glReadPixels(0,0,1280,720,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        std::ofstream shot("build/village-smoke.ppm",std::ios::binary);
        shot<<"P6\n1280 720\n255\n";
        for(int y=719;y>=0;--y) shot.write(reinterpret_cast<char*>(pixels.data()+y*1280*3),1280*3);
        std::cout<<"Village smoke: doorway open, wall collision active, GL error="<<error<<"\n";
        return error==GL_NO_ERROR?0:4;
    }

    // Enter the engine main loop
    engine.mainloop();

    return 0;
}
