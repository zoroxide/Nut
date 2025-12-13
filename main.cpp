#include "Nut/Nut.h"
#include <iostream>

using namespace std;

int main() {
    Engine engine;

    if (!engine.init(true)) {
        cerr << "Failed to initialize engine\n";
        return -1;
    }

    // engine.load_terrain_using_texture("assets/textures/grass.png");
    engine.load_flat_terrain("assets/textures/grass.png");
    engine.vsync(true);

    engine.mainloop();

    return 0;
}
