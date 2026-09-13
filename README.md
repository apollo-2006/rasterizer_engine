# rasterizer_engine

[![demo](https://github.com/apollo-2006/rasterizer_engine/actions/workflows/pages.yml/badge.svg)](https://github.com/apollo-2006/rasterizer_engine/actions/workflows/pages.yml)
[![license: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A software rasterizer in C++17. It takes a 3D mesh, runs it through a perspective
projection matrix written out longhand, culls and shades each face, and fills and
depth-tests every pixel itself into a CPU framebuffer. SDL2 only opens the window and
receives one finished texture per frame.

**[Run it in your browser →](https://apollo-2006.github.io/rasterizer_engine/)** The same
`src/main.cpp`, compiled against Emscripten's SDL2 port, with switches for culling, a
depth buffer view, and live CPU time per frame.

It is the C++ counterpart to [cpu_rasterizer](https://github.com/apollo-2006/cpu_rasterizer),
which does the same pipeline in JavaScript.

## The pipeline

Each frame walks every triangle of a cube through the stages a GPU would, in order:

1. **Model transform.** Each vertex is rotated about Y and X with plain trig, then
   translated 4 units down +Z so the cube sits in front of the camera.
2. **Back-face culling.** The face normal is the cross product of two edges. The camera
   is at the origin, so a face whose normal points away from the vector to it cannot be
   seen and is dropped before it costs anything: 12 triangles become at most 6.
3. **Flat shading.** One Lambert term per face, the dot product of the normal with a fixed
   light direction, with a floor so faces turned away stay visible.
4. **Projection.** `mat4::perspective` builds the matrix from field of view, aspect ratio
   and the near and far planes. `multiply_vector` treats the `vec3` as a 4D vector with
   `w = 1` and returns the `w` the matrix produced.
5. **Perspective divide and viewport.** Dividing x and y by `w` is what makes distant
   geometry smaller. NDC in `[-1, +1]` is then mapped to pixels, with y flipped because
   screen rows grow downward.
6. **Rasterization.** `draw_triangle` walks the triangle's screen-space bounding box and
   evaluates three edge functions at each pixel centre. Divided by the triangle's signed
   area they are the barycentric weights, so one computation decides coverage for either
   winding order and interpolates depth.
7. **Depth test.** The value interpolated and stored per pixel is `1/w`, because `1/w` is
   linear in screen space and post-divide z is not. Larger is nearer; the buffer clears
   to 0.

Press **C** to toggle culling, **Z** to show the depth buffer, **space** to pause. With
culling off the image does not change, since the depth buffer hides the back faces
anyway; the frame just does twice the rasterization to get there.

## Performance

Ryzen 9 5900XT, g++ `-O3`, one core, the 800×600 frame of the rotating cube, CPU work
only (transform through depth test, not the SDL upload), mean over 3000 frames:

| | per frame | frames/s of CPU work |
|---|---|---|
| culling on | **0.51 ms** | ~1,970 |
| culling off | 0.99 ms | ~1,010 |

The earlier version drew each pixel with its own `SDL_RenderDrawPoint` call. Filling a
framebuffer and uploading it once is what makes the browser build usable at all, where
every SDL call crosses from WebAssembly into JavaScript.

## Build & run

Requires SDL2 development headers.

```bash
# Debian/Ubuntu: sudo apt install libsdl2-dev    Arch: sudo pacman -S sdl2
git clone https://github.com/apollo-2006/rasterizer_engine.git
cd rasterizer_engine

make
./forge_engine
```

## Web demo

`web/build.sh` compiles `src/main.cpp` with `em++ -sUSE_SDL=2`. The only browser-specific
code is behind `#ifdef __EMSCRIPTEN__`: the browser owns the frame loop, so `main` hands
`frame()` to `emscripten_set_main_loop` instead of looping itself, and the page's switches
call four exported setters. GitHub Actions builds the demo and publishes it to Pages on
every push to `main`.

```bash
web/build.sh                          # needs em++ on PATH
python3 -m http.server -d web/dist    # then open http://localhost:8000
```

## Layout

```
src/main.cpp          window, frame loop, the pipeline, the rasterizer
include/vec3.hpp      3D vector, dot and cross product
include/mat4.hpp      4x4 matrix, perspective construction, vector transform
include/triangle.hpp  three vertices and a flat colour
web/                  Emscripten build script and demo page
```

## Known limits

* **One hardcoded mesh.** A cube built in `make_cube()`; there is no model loading.
* **Flat colour per face.** No per-vertex attributes, texturing or smooth shading. The
  barycentric weights interpolate depth only.
* **No near-plane clipping.** A triangle with a vertex behind the camera is dropped
  whole rather than clipped.
* **One thread.** The bounding-box loop would split across rows cleanly, but it does not.

## License

MIT. See [LICENSE](LICENSE).

## Author

**Abir Deol** · [abirdeol.tech](https://abirdeol.tech)
