#include "Nut/Nut.h"
#include <iostream>

int main() {
    Engine engine;

    // Initialize the engine (fullscreen by default). If you want windowed, pass false.
    if (!engine.init(true)) {
        std::cerr << "Failed to initialize engine\n";
        return -1;
    }

    // Load terrain texture
    engine.load_terrain_using_texture("assets/grass.png");

    // Load panorama texture (optional)
    if (!engine.panorama("assets/qwantani_moon_noon_puresky_4k.hdr")) {
        std::cerr << "Failed to load panorama texture\n";
    }

    // Toggle vsync if desired
    engine.vsync(true);

    // Enter the engine main loop
    engine.mainloop();

    return 0;
}
