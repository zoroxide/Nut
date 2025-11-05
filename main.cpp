#include "Nut/Nut.h"
#include <iostream>

int main() {
    Engine engine;

    if (!engine.init(true)) {
        std::cerr << "Failed to initialize engine\n";
        return -1;
    }

    engine.load_terrain_using_texture("assets/textures/grass.png");

    if (!engine.panorama("assets/skybox/Daylight Box UV.png")) {
        std::cerr << "Failed to load skybox (provide folder of faces or single equirectangular .png)\n";
    }

    engine.vsync(true);

    engine.mainloop();

    return 0;
}
