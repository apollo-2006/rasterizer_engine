CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3 $(shell sdl2-config --cflags)
LDLIBS = $(shell sdl2-config --libs)

forge_engine: src/main.cpp include/vec3.hpp include/mat4.hpp include/triangle.hpp
	$(CXX) $(CXXFLAGS) -Iinclude src/main.cpp -o $@ $(LDLIBS)

clean:
	rm -f forge_engine

.PHONY: clean
