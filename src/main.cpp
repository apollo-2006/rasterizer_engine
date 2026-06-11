#include "../include/vec3.hpp"
#include "../include/mat4.hpp"
#include "../include/triangle.hpp"
#include <SDL2/SDL.h>
#include <vector>
#include <iostream>
#include <algorithm>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Barycentric algorithm to determine if a 2D point (x,y) is inside a 2D triangle
bool is_point_in_triangle(int x, int y, const vec3& v0, const vec3& v1, const vec3& v2) {
    auto edge_func = [](const vec3& a, const vec3& b, const vec3& c) {
        return (c.x() - a.x()) * (b.y() - a.y()) - (c.y() - a.y()) * (b.x() - a.x());
    };

    vec3 p(x, y, 0);
    double w0 = edge_func(v1, v2, p);
    double w1 = edge_func(v2, v0, p);
    double w2 = edge_func(v0, v1, p);

    return (w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0);
}

// Bounding box rasterizer
void draw_filled_triangle(SDL_Renderer* renderer, const triangle& tri) {
    // 1. Find the 2D bounding box of the triangle on the screen
    int min_x = std::max(0, static_cast<int>(std::min({tri.p[0].x(), tri.p[1].x(), tri.p[2].x()})));
    int min_y = std::max(0, static_cast<int>(std::min({tri.p[0].y(), tri.p[1].y(), tri.p[2].y()})));
    int max_x = std::min(WINDOW_WIDTH - 1, static_cast<int>(std::max({tri.p[0].x(), tri.p[1].x(), tri.p[2].x()})));
    int max_y = std::min(WINDOW_HEIGHT - 1, static_cast<int>(std::max({tri.p[0].y(), tri.p[1].y(), tri.p[2].y()})));

    SDL_SetRenderDrawColor(renderer, tri.color.x(), tri.color.y(), tri.color.z(), 255);

    // 2. Loop over every pixel in the bounding box
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            // 3. If the pixel is inside the math boundaries, color it
            if (is_point_in_triangle(x, y, tri.p[0], tri.p[1], tri.p[2])) {
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Software Rasterizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Create our projection matrix
    mat4 proj_matrix = mat4::perspective(90.0, (double)WINDOW_HEIGHT / (double)WINDOW_WIDTH, 0.1, 1000.0);

    // Define a 3D triangle in the world
    triangle mesh_tri(
        vec3(0.0, 1.0, 0.0),   // Top vertex
        vec3(1.0, -1.0, 0.0),  // Bottom right
        vec3(-1.0, -1.0, 0.0), // Bottom left
        vec3(0, 255, 100)      // Neon green color
    );

    double time = 0.0;
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e)) { if (e.type == SDL_QUIT) quit = true; }

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        time += 0.01;

        // Push the triangle back in Z-space so the camera can see it
        triangle projected_tri = mesh_tri;
        for (int i = 0; i < 3; i++) {
            vec3 v = projected_tri.p[i];

            // Simple Z-axis translation and Y-axis rotation using basic trig
            double rotated_x = v.x() * std::cos(time) - v.z() * std::sin(time);
            double rotated_z = v.x() * std::sin(time) + v.z() * std::cos(time);
            v.e[0] = rotated_x;
            v.e[2] = rotated_z + 3.0; // Push 3 units deep into the screen

            // Execute the Graphics Pipeline
            double w = 1.0;
            vec3 projected = proj_matrix.multiply_vector(v, w);

            // Perspective Divide (The core of 3D projection)
            if (w != 0.0) {
                projected.e[0] /= w;
                projected.e[1] /= w;
                projected.e[2] /= w;
            }

            // Scale from normalized math space (-1 to +1) to actual Screen Pixels
            projected.e[0] += 1.0; projected.e[1] += 1.0;
            projected.e[0] *= 0.5 * WINDOW_WIDTH;
            projected.e[1] *= 0.5 * WINDOW_HEIGHT;

            projected_tri.p[i] = projected;
        }

        // Draw the math to the screen
        draw_filled_triangle(renderer, projected_tri);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}