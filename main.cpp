#include "Engine/Engine.h"
#include <iostream>

int main() {
    Engine engine;

    // Initialize the engine (fullscreen by default). If you want windowed, pass false.
    if (!engine.init(true)) {
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

    // Enter the engine main loop
    engine.mainloop();

    return 0;
}
