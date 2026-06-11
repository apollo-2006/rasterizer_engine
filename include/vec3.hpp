#pragma once
#include <cmath>
#include <random>

// Thread-safe random double generator [0.0, 1.0)
inline double random_double() {
    thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(generator);
}

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

class vec3 {
public:
    double e[3];

    vec3() : e{0,0,0} {}
    vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }

    vec3 operator+(const vec3& v) const {
        return vec3(e[0] + v.e[0], e[1] + v.e[1], e[2] + v.e[2]);
    }

    vec3 operator-(const vec3& v) const {
        return vec3(e[0] - v.e[0], e[1] - v.e[1], e[2] - v.e[2]);
    }

    vec3 operator*(const vec3& v) const {
        return vec3(e[0] * v.e[0], e[1] * v.e[1], e[2] * v.e[2]);
    }

    vec3 operator*(double t) const {
        return vec3(e[0] * t, e[1] * t, e[2] * t);
    }

    vec3 operator/(double t) const {
        return *this * (1.0 / t);
    }

    inline static double dot(const vec3& u, const vec3& v) {
        return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
    }

    inline vec3 normalize() const {
        double length = std::sqrt(e[0]*e[0] + e[1]*e[1] + e[2]*e[2]);
        return vec3(e[0]/length, e[1]/length, e[2]/length);
    }

    // Generate random vectors for anti-aliasing and lighting
    inline static vec3 random() {
        return vec3(random_double(), random_double(), random_double());
    }

    inline static vec3 random(double min, double max) {
        return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
    }
};

inline vec3 operator*(double t, const vec3& v) {
    return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}

// Utility to generate a random point inside a 3D sphere for diffuse lighting
inline vec3 random_in_unit_sphere() {
    while (true) {
        auto p = vec3::random(-1.0, 1.0);
        if (vec3::dot(p, p) >= 1.0) continue;
        return p;
    }
}

// Clamps a value between a min and max
inline double clamp(double x, double min, double max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

using point3 = vec3;
using color = vec3;