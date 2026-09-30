# **Procedural Terrain Generator Based Game using Moden OpenGL**
A beautifull 3D Fixed Terrain Generation (Perlin Noise) based game (graphics engine with movement controls)
Created using Modern OpenGL (GLFW, GLEW, GLM), modern C++ and finally stb_image for image handling and others..

### Thanks to:
**this software was never be produced without these resources:**
 - [learnopengl.com](https://learnopengl.com/)
 - [OGLDEV](https://www.youtube.com/@OGLDEV)
 - CS633 / CS352 Computer Graphics & linear algebra college courses

# Screenshots
new verion:
<img width="1920" height="1080" alt="Screenshot From 2026-09-30 11-52-11" src="https://github.com/user-attachments/assets/6d710708-2c6d-4d0d-bae0-645753addcd3" />
<img width="1920" height="1080" alt="Screenshot From 2026-09-30 11-54-03" src="https://github.com/user-attachments/assets/df1cdf76-f1b2-4c4d-9ac5-2cea998f0848" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/3b43c0d8-952c-44d3-b254-6d54c94634bd" />
<img width="1920" height="1080" alt="Screenshot From 2026-09-30 10-48-17" src="https://github.com/user-attachments/assets/8c3fd254-9e4b-44ee-a6c3-abd552941da7" />
old version:
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/f3ffdeab-faa8-443a-b2bd-3d36c32b81de" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/242c0160-3348-43c3-896a-9da0c6687416" />
<img width="1366" height="768" alt="Screenshot From 2026-09-13 04-54-36" src="https://github.com/user-attachments/assets/08c704ee-2f23-49a2-ac3d-a3ad25abd3ef" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/d0fcbf73-9daa-42e4-8045-116761a2fe4d" />
<img width="1927" height="1080" alt="image" src="https://github.com/user-attachments/assets/50be3578-bbae-4739-96e2-42976e7c3efa" />

# Demo Code
use you own textures and Panoramas (png and HDR)

```cpp
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


```

# Controls
- **WASD** for moving (**Shift** to sprint / swim faster)
- **SPACE_BAR** for jumping (swim up while in the water)
- **C** or **Ctrl** to dive while swimming (or look down and press **W**)
- **Mouse** cursor for Looking
- **Tab** to show / hide the settings panel (frees the mouse while it is open)
- **M** to switch the minimap (bottom-left) between small and large
- **Enter** to free / capture the mouse

# World
- A procedurally generated island (hills, mountains, beaches, no lakes) surrounded by an ocean
- Ocean with Gerstner waves: walk into the sea to swim, dive to explore the sea floor,
  and keep an eye on your oxygen
- Swaying forests (fir, broadleaf and acacia trees) and wind-blown grass that parts as you walk through it
- Sun shadows from the terrain and trees, valley fog, sun glow in the haze, bloom and sun rays
- Terrain presets, island shape, waves, materials, vegetation, sky, fog and graphics quality are all tweakable in the GUI

# Performance
Built to hold 60 FPS on integrated graphics (tested on Intel HD 620, 1920x1080):
chunked terrain LOD, compressed textures, billboard trees in the distance, and an adaptive render
resolution (Tab -> Graphics & Performance: quality preset, target FPS, per-pass GPU timings)

# Assets
HDRI skies (`assets/panoramas`), terrain materials (`assets/textures/terrain`) and the bark / leaf textures
the tree cards are baked from (`assets/textures/foliage`) are CC0 from [Poly Haven](https://polyhaven.com)
  
# Installing Requirements (only linux for now, never tested on Windows or Mac) :

### Install Libs
```
sudo apt update
sudo apt install -y build-essential g++ cmake pkg-config git make cmake
sudo apt install -y libglfw3-dev libglew-dev libglm-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev libassimp-dev

```

### compile and run: 
```
sudo make run
```

