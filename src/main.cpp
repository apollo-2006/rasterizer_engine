#include "vec3.hpp"
#include "mat4.hpp"
#include "triangle.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// The whole frame is drawn into these on the CPU, then uploaded to SDL as one
// texture. SDL never draws a pixel of the image itself.
std::vector<uint32_t> framebuffer(WINDOW_WIDTH * WINDOW_HEIGHT);
std::vector<float> depthbuffer(WINDOW_WIDTH * WINDOW_HEIGHT);

// A unit cube around the origin, two triangles per face, wound counter-clockwise
// when seen from outside so the geometric normal points outward.
std::vector<triangle> make_cube() {
    const vec3 p[8] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1},
    };
    const int faces[6][4] = {
        {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3}, {1, 2, 6, 5}, {3, 7, 6, 2}, {0, 1, 5, 4},
    };
    const vec3 colors[6] = {
        {0, 255, 100}, {0, 200, 255}, {255, 180, 60}, {230, 90, 220}, {240, 240, 240}, {255, 90, 90},
    };
    std::vector<triangle> mesh;
    for (int f = 0; f < 6; f++) {
        const auto& q = faces[f];
        mesh.emplace_back(p[q[0]], p[q[1]], p[q[2]], colors[f]);
        mesh.emplace_back(p[q[0]], p[q[2]], p[q[3]], colors[f]);
    }
    return mesh;
}

// A vertex after projection: pixel position, and 1/w for depth. 1/w is what
// interpolates linearly across the screen; post-divide z does not.
struct screen_vertex {
    double x, y, inv_w;
};

struct options {
    bool culling = true;
    bool depth_view = false;
    bool paused = false;
};

options opts;
double frame_ms = 0;
int triangles_drawn = 0;

void clear_buffers() {
    std::fill(framebuffer.begin(), framebuffer.end(), 0xFF070B12);  // the demo page's background
    std::fill(depthbuffer.begin(), depthbuffer.end(), 0.0f);  // 0 = infinitely far
}

// Bounding box rasterizer with edge functions. The three edge values, divided by
// the triangle's signed area, are its barycentric weights: the same numbers decide
// coverage and interpolate depth.
void draw_triangle(const screen_vertex& v0, const screen_vertex& v1, const screen_vertex& v2,
                   uint32_t argb) {
    auto edge = [](const screen_vertex& a, const screen_vertex& b, double x, double y) {
        return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
    };

    const double area = edge(v0, v1, v2.x, v2.y);
    if (area == 0) return;

    int min_x = std::max(0, static_cast<int>(std::floor(std::min({v0.x, v1.x, v2.x}))));
    int min_y = std::max(0, static_cast<int>(std::floor(std::min({v0.y, v1.y, v2.y}))));
    int max_x = std::min(WINDOW_WIDTH - 1, static_cast<int>(std::ceil(std::max({v0.x, v1.x, v2.x}))));
    int max_y = std::min(WINDOW_HEIGHT - 1, static_cast<int>(std::ceil(std::max({v0.y, v1.y, v2.y}))));

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            // Sample at the pixel centre. Dividing by the signed area makes the
            // inside test the same for either winding order.
            const double px = x + 0.5, py = y + 0.5;
            const double w0 = edge(v1, v2, px, py) / area;
            const double w1 = edge(v2, v0, px, py) / area;
            const double w2 = 1.0 - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            const float depth = static_cast<float>(w0 * v0.inv_w + w1 * v1.inv_w + w2 * v2.inv_w);
            const int i = y * WINDOW_WIDTH + x;
            if (depth <= depthbuffer[i]) continue;  // larger 1/w is closer
            depthbuffer[i] = depth;
            framebuffer[i] = argb;
        }
    }
}

// Greyscale view of the depth buffer: nearer is brighter.
void show_depth() {
    float lo = 1e30f, hi = 0;
    for (float d : depthbuffer) if (d > 0) { lo = std::min(lo, d); hi = std::max(hi, d); }
    const float span = hi > lo ? hi - lo : 1;
    for (size_t i = 0; i < depthbuffer.size(); i++) {
        if (depthbuffer[i] <= 0) continue;
        const uint32_t v = 40 + static_cast<uint32_t>(215 * (depthbuffer[i] - lo) / span);
        framebuffer[i] = 0xFF000000 | (v << 16) | (v << 8) | v;
    }
}

void render(const std::vector<triangle>& mesh, const mat4& proj, double time) {
    clear_buffers();
    triangles_drawn = 0;

    const double cy = std::cos(time), sy = std::sin(time);
    const double cx = std::cos(time * 0.6), sx = std::sin(time * 0.6);
    // Direction toward the light: up, right, and back toward the camera.
    const vec3 light = vec3(0.4, 0.7, -1.0).normalize();

    for (const triangle& tri : mesh) {
        // 1. Model transform: rotate about Y, then X, then push 4 units into the screen.
        vec3 world[3];
        for (int k = 0; k < 3; k++) {
            const vec3& v = tri.p[k];
            const double x1 = v.x() * cy + v.z() * sy;
            const double z1 = -v.x() * sy + v.z() * cy;
            const double y2 = v.y() * cx - z1 * sx;
            const double z2 = v.y() * sx + z1 * cx;
            world[k] = vec3(x1, y2, z2 + 4.0);
        }

        // 2. Back-face culling in view space. The camera sits at the origin, so a
        //    face whose normal points away from the ray to it cannot be seen.
        const vec3 normal = vec3::cross(world[1] - world[0], world[2] - world[0]).normalize();
        if (opts.culling && vec3::dot(normal, world[0]) >= 0) continue;

        // 3. Flat shading: one Lambert term for the whole face, with a floor so
        //    faces turned from the light stay visible.
        const double lambert = std::max(0.18, vec3::dot(normal, light));
        const uint32_t argb = 0xFF000000 |
            (static_cast<uint32_t>(tri.color.x() * lambert) << 16) |
            (static_cast<uint32_t>(tri.color.y() * lambert) << 8) |
            static_cast<uint32_t>(tri.color.z() * lambert);

        // 4. Projection, perspective divide, viewport. Screen y grows downward, so
        //    NDC y is flipped on the way to pixels.
        screen_vertex sv[3];
        bool behind = false;
        for (int k = 0; k < 3; k++) {
            double w = 1.0;
            const vec3 clip = proj.multiply_vector(world[k], w);
            if (w <= 0.0) { behind = true; break; }
            sv[k].x = (clip.x() / w + 1.0) * 0.5 * WINDOW_WIDTH;
            sv[k].y = (1.0 - clip.y() / w) * 0.5 * WINDOW_HEIGHT;
            sv[k].inv_w = 1.0 / w;
        }
        if (behind) continue;  // no near-plane clipping: drop instead of smearing

        // 5. Rasterize with the depth test.
        draw_triangle(sv[0], sv[1], sv[2], argb);
        triangles_drawn++;
    }

    if (opts.depth_view) show_depth();
}

struct app {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    std::vector<triangle> mesh = make_cube();
    mat4 proj = mat4::perspective(70.0, (double)WINDOW_HEIGHT / (double)WINDOW_WIDTH, 0.1, 1000.0);
    double time = 0.7;  // start at an angle, so the first frame already reads as a cube
    bool quit = false;
};

app g;

void frame() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) g.quit = true;
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_c: opts.culling = !opts.culling; break;
                case SDLK_z: opts.depth_view = !opts.depth_view; break;
                case SDLK_SPACE: opts.paused = !opts.paused; break;
                case SDLK_ESCAPE: g.quit = true; break;
            }
        }
    }

    const Uint64 t0 = SDL_GetPerformanceCounter();
    if (!opts.paused) g.time += 0.01;
    render(g.mesh, g.proj, g.time);
    frame_ms = 1000.0 * (SDL_GetPerformanceCounter() - t0) / SDL_GetPerformanceFrequency();

    SDL_UpdateTexture(g.texture, nullptr, framebuffer.data(), WINDOW_WIDTH * sizeof(uint32_t));
    SDL_RenderCopy(g.renderer, g.texture, nullptr, nullptr);
    SDL_RenderPresent(g.renderer);
}

#ifdef __EMSCRIPTEN__
extern "C" {
EMSCRIPTEN_KEEPALIVE void set_culling(int on) { opts.culling = on; }
EMSCRIPTEN_KEEPALIVE void set_depth_view(int on) { opts.depth_view = on; }
EMSCRIPTEN_KEEPALIVE void set_paused(int on) { opts.paused = on; }
EMSCRIPTEN_KEEPALIVE double get_frame_ms() { return frame_ms; }
EMSCRIPTEN_KEEPALIVE int get_triangles_drawn() { return triangles_drawn; }
}
#endif

int main() {
    // Every one of these can fail (no display, no video driver, headless session).
    // Unchecked, the first null pointer reaches SDL as a segfault with nothing to read.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    g.window = SDL_CreateWindow("Software Rasterizer  [C] culling  [Z] depth  [space] pause",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!g.window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    g.renderer = SDL_CreateRenderer(g.window, -1, SDL_RENDERER_ACCELERATED);
    if (!g.renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(g.window);
        SDL_Quit();
        return 1;
    }

    g.texture = SDL_CreateTexture(g.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                  WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!g.texture) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(g.renderer);
        SDL_DestroyWindow(g.window);
        SDL_Quit();
        return 1;
    }

#ifdef __EMSCRIPTEN__
    // The browser owns the loop; it calls frame() once per display refresh.
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (!g.quit) {
        frame();
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyTexture(g.texture);
    SDL_DestroyRenderer(g.renderer);
    SDL_DestroyWindow(g.window);
    SDL_Quit();
#endif
    return 0;
}
