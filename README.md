# rasterizer_engine
A software rasterizer built from scratch in C++ that takes 3D points, multiplies them by a 4x4 Perspective Matrix to flatten them onto a 2D screen, and fills in the pixels between those points. Built to understand how rendering pipelines work at the lowest level — before any GPU abstractions.

## Features
* **Perspective Projection**: Flattens 3D coordinates onto a 2D plane using a 4x4 Perspective Matrix.
* **Triangle Rasterization**: Fills in the pixels between projected 3D points using `draw_filled_triangle` to render shapes.
* **SDL2 Backend**: Utilizes the Simple DirectMedia Layer (SDL2) to create windows, render points, and handle application events.
* **Optimized C++**: Built using the C++17 standard and compiled with the `-O3` optimization flag for high-performance rendering.

## Build & Run
```bash
# Compile the source files into the obj/ directory and link the executable
make

# Run the rasterizer
./forge_engine

# Clean build artifacts
make clean
```

## Dependencies
* SDL2 (`libSDL2-2.0.so.0`)
* C++17 standard library
* Standard math libraries

## Author
**Abir Deol**
