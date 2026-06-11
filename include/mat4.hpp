#pragma once
#include "vec3.hpp"
#include <cmath>
#include <cstring>

class mat4 {
public:
    double m[4][4];

    mat4() { std::memset(m, 0, sizeof(m)); }

    // Creates an Identity Matrix (the matrix equivalent of the number '1')
    static mat4 identity() {
        mat4 mat;
        mat.m[0][0] = 1.0; mat.m[1][1] = 1.0;
        mat.m[2][2] = 1.0; mat.m[3][3] = 1.0;
        return mat;
    }

    // Creates the Perspective Projection Matrix
    static mat4 perspective(double fov_degrees, double aspect_ratio, double z_near, double z_far) {
        mat4 mat;
        double fov_rad = 1.0 / std::tan(fov_degrees * 0.5 / 180.0 * 3.14159);

        mat.m[0][0] = aspect_ratio * fov_rad;
        mat.m[1][1] = fov_rad;
        mat.m[2][2] = z_far / (z_far - z_near);
        mat.m[3][2] = (-z_far * z_near) / (z_far - z_near);
        mat.m[2][3] = 1.0;
        mat.m[3][3] = 0.0;
        return mat;
    }

    // Matrix-Vector Multiplication
    // We pass in a vec3, but internally treat it as a 4D vector (x, y, z, w=1)
    vec3 multiply_vector(const vec3& i, double& w) const {
        vec3 o;
        o.e[0] = i.x() * m[0][0] + i.y() * m[1][0] + i.z() * m[2][0] + m[3][0];
        o.e[1] = i.x() * m[0][1] + i.y() * m[1][1] + i.z() * m[2][1] + m[3][1];
        o.e[2] = i.x() * m[0][2] + i.y() * m[1][2] + i.z() * m[2][2] + m[3][2];
        w      = i.x() * m[0][3] + i.y() * m[1][3] + i.z() * m[2][3] + m[3][3];
        return o;
    }
};