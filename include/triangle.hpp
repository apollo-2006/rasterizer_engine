#pragma once
#include "vec3.hpp"

struct triangle {
    vec3 p[3];
    vec3 color; // Simple flat color for the rasterizer

    triangle() {}
    triangle(vec3 p1, vec3 p2, vec3 p3, vec3 c) {
        p[0] = p1; p[1] = p2; p[2] = p3;
        color = c;
    }
};