# opengl-terrain-generator-and-renderer

Created using Modern OpenGL (GLFW, GLEW, GLM), modern C++ and finally stb_image for image handling 

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/823029fa-c3c7-46e4-970b-842b513b22a7" />

# Controls
- **WASD** for moving
- **SPACE_BAR** for jumping
- **Mouse** cursor for Looking
  
# Installing Requirements:

### Install Libs
```
sudo apt update
sudo apt install -y build-essential g++ cmake pkg-config git
sudo apt install -y libglfw3-dev libglew-dev libglm-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

### compile and run: 
```
make run
```

### Manual Compilation:
```
g++ -std=c++17 -Wall src/main.cpp -o build/program -lglfw -lGL -lGLEW
./build/program
```
