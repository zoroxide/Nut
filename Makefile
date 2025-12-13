CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./Nut -I./Nut/gui -I./Nut/libs/imgui -I./Nut/libs/imgui/backends
LIBS = -lGLEW -lglfw -lGL -ldl -lpthread -lm -lassimp -Wl,--copy-dt-needed-entries

SRC = main.cpp \
      Nut/Nut.cpp \
      Nut/Camera.cpp \
      Nut/Shaders.cpp \
      Nut/ECS.cpp \
      Nut/Terrain.cpp \
      Nut/Skybox.cpp \
      Nut/Models.cpp \
      Nut/Renderer.cpp \
      Nut/gui/gui.cpp \
      Nut/libs/stb_image.cpp \
      Nut/libs/imgui/imgui.cpp \
      Nut/libs/imgui/imgui_draw.cpp \
      Nut/libs/imgui/imgui_tables.cpp \
      Nut/libs/imgui/imgui_widgets.cpp \
      Nut/libs/imgui/backends/imgui_impl_glfw.cpp \
      Nut/libs/imgui/backends/imgui_impl_opengl3.cpp

# IMGUI_SRC = libs/imgui/imgui.cpp libs/imgui/backends/imgui_impl_glfw.cpp libs/imgui/backends/imgui_impl_opengl3.cpp
# SRC += $(IMGUI_SRC)

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
