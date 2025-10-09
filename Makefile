CXX = g++
CXXFLAGS = -std=c++17 -Wall
LIBS = -lglfw -lGL -lGLEW
SRC = src/main.cpp
OUT_DIR = build
OUT = $(OUT_DIR)/program

$(OUT): $(SRC)
	mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LIBS)

run: $(OUT)
	./$(OUT)

clean:
	rm -rf $(OUT_DIR)
