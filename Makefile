# Voxel World Builder
#   make            build everything that needs no libraries (CP1 renderer + CP2 animator)
#   make run        CP1: render the technique demos into outputs/cp1/
#   make animate    CP2: render transformation pages and animation frames into outputs/cp2/
#   make engine     CP3: build the interactive OpenGL prototype (needs GLFW + GLEW)
#   make play       build and start the OpenGL prototype
#   make clean

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CPPFLAGS += -Isrc

SHARED   := $(wildcard src/core/*.cpp src/raster/*.cpp src/color/*.cpp src/math/*.cpp \
                       src/anim/*.cpp src/scene/*.cpp src/ui/*.cpp)
DEMOS    := $(wildcard src/demos/*.cpp)
ENGINE   := $(wildcard src/engine/*.cpp)
HEADERS  := $(wildcard src/*/*.h)

.PHONY: all run animate engine play clean

all: bin/renderer bin/animator

bin/renderer: src/apps/renderer.cpp $(SHARED) $(DEMOS) $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) src/apps/renderer.cpp $(SHARED) $(DEMOS) -o $@

bin/animator: src/apps/animator.cpp $(SHARED) $(DEMOS) $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) src/apps/animator.cpp $(SHARED) $(DEMOS) -o $@

run: bin/renderer
	./bin/renderer outputs/cp1

animate: bin/animator
	./bin/animator outputs/cp2

engine: bin/engine

bin/engine: src/apps/engine.cpp $(ENGINE) $(SHARED) $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -Wno-unused-parameter -Ithird_party src/apps/engine.cpp $(ENGINE) $(SHARED) -o $@ -lglfw -lGLEW -lGL

play: bin/engine
	./bin/engine

clean:
	rm -rf bin
