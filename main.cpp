#include "Nut/Nut.h"
#include <iostream>

int main() {
    Engine engine;

    if (!engine.init(true)) {
        std::cerr << "Failed to initialize engine\n";
        return -1;
    }

    engine.load_terrain_using_texture("assets/grass.png");

    if (!engine.panorama("assets/belfast_sunset_puresky_4k.hdr")) {
        std::cerr << "Failed to load panorama texture\n";
    }

    engine.vsync(true);

    engine.mainloop();

    return 0;
}
