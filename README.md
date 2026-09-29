# **Nut**

Simple and beautifull 3D Fixed Terrain Generation (Perlin Noise) based game
Created using Modern OpenGL (GLFW, GLEW, GLM), modern C++ and finally stb_image for image handling 

# Screenshots
### using HDR panorama
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/f3ffdeab-faa8-443a-b2bd-3d36c32b81de" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/242c0160-3348-43c3-896a-9da0c6687416" />
<img width="1366" height="768" alt="Screenshot From 2026-09-13 04-54-36" src="https://github.com/user-attachments/assets/08c704ee-2f23-49a2-ac3d-a3ad25abd3ef" />


### with GUI (Dear Imgui)
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

    // Load panorama (optional). HDR panoramas also drive the sun direction, colour and fog.
    if (!engine.panorama("assets/panoramas/kloofendal_48d_partly_cloudy_puresky_4k.hdr")) {
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
- **Enter** for Free mouse to use GUI Controlls

# World
- A procedurally generated island (hills, mountains, beaches, no lakes) surrounded by an ocean
- Ocean with Gerstner waves: walk into the sea to swim, dive to explore the sea floor,
  and keep an eye on your oxygen
- Terrain presets, island shape, waves, materials, sky and fog are all tweakable in the GUI

# Assets
HDRI skies (`assets/panoramas`) and terrain materials (`assets/textures/terrain`) are CC0 from [Poly Haven](https://polyhaven.com).
  
# Installing Requirements:

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

