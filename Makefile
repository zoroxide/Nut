CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./Engine -I./Engine/gui -I./Engine/libs/imgui -I./Engine/libs/imgui/backends
LIBS = -lGLEW -lglfw -lGL -ldl -lpthread -lm -lassimp -Wl,--copy-dt-needed-entries

SRC = main.cpp \
      Engine/Engine.cpp \
      Engine/Camera.cpp \
      Engine/Shaders.cpp \
      Engine/ECS.cpp \
      Engine/Terrain.cpp \
      Engine/Skybox.cpp \
      Engine/Models.cpp \
      Engine/Renderer.cpp \
      Engine/gui/gui.cpp \
      Engine/Coins.cpp \
      Engine/libs/stb_image.cpp \
      Engine/libs/imgui/imgui.cpp \
      Engine/libs/imgui/imgui_draw.cpp \
      Engine/libs/imgui/imgui_tables.cpp \
      Engine/libs/imgui/imgui_widgets.cpp \
      Engine/libs/imgui/backends/imgui_impl_glfw.cpp \
      Engine/libs/imgui/backends/imgui_impl_opengl3.cpp

OUT_DIR = build
TARGET = program
OUT = $(OUT_DIR)/$(TARGET)

$(OUT): $(SRC)
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LIBS)

run: $(OUT)
	./$(OUT)

.PHONY: run clean

clean:
	rm -rf $(OUT_DIR)
