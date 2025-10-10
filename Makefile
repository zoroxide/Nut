CXX = g++
CXXFLAGS = -std=c++17 -Wall
LIBS = -lGLEW -lglfw -lGL -ldl -lpthread -lm
SRC = main.cpp Nut/Nut.cpp
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
