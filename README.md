# rasterizer_engine

A software rasterizer in C++17. It takes 3D triangles, runs them through a perspective
projection matrix written out longhand, and fills the pixels between the projected
vertices itself. SDL2 is used only to open a window and set individual pixels — every
step between a vertex in world space and a lit pixel is done by hand.

## The pipeline

Each frame walks a vertex through the same stages a GPU would, in the same order:

1. **Model transform.** The triangle is rotated about the Y axis with plain trig, then
   translated 3 units down +Z so it sits in front of the camera.
2. **Projection.** `mat4::perspective` builds the projection matrix from field of view,
   aspect ratio and the near/far planes. `multiply_vector` treats the `vec3` as a 4D
   vector with `w = 1` and returns the transformed point along with the `w` that the
   matrix produced.
3. **Perspective divide.** Dividing x, y and z by that `w` is what actually makes distant
   geometry smaller. This is the single step that separates a perspective projection
   from an orthographic one.
4. **Viewport transform.** Normalized device coordinates in `[-1, +1]` are shifted and
   scaled into pixel coordinates.
5. **Rasterization.** `draw_filled_triangle` computes the triangle's screen-space
   bounding box, then tests each pixel in it with three edge functions. A pixel is
   inside when all three signed areas share a sign, which handles either winding order
   without a special case.

## Build & run

Requires SDL2 development headers.

```bash
# Debian/Ubuntu
sudo apt install libsdl2-dev

git clone https://github.com/apollo-2006/rasterizer_engine.git
cd rasterizer_engine

make
./forge_engine
```

A green triangle spins about its Y axis on a dark background at roughly 60 FPS. Close
the window or Ctrl-C to quit.

## Layout

```
src/main.cpp          window, frame loop, projection, triangle fill
include/vec3.hpp      3D vector type and math helpers
include/mat4.hpp      4x4 matrix, perspective construction, vector transform
include/triangle.hpp  three vertices and a flat colour
```

## Known limits

* **One hardcoded triangle.** There is no mesh format or model loading; the geometry is
  written into `main.cpp`.
* **No depth buffer.** Triangles are drawn in the order given, so with more than one
  piece of geometry the later one always wins regardless of depth.
* **Flat colour only.** No lighting, texturing, or per-vertex attribute interpolation —
  the edge functions decide coverage but their barycentric weights are discarded rather
  than used to interpolate across the surface.
* **No back-face culling or near-plane clipping.** Geometry that crosses the camera
  plane will produce garbage rather than being clipped.
* **Per-pixel `SDL_RenderDrawPoint`.** One draw call per pixel is the slow way to do
  this; a CPU-side framebuffer uploaded as a single texture would be far faster.

## Author

**Abir Deol**
