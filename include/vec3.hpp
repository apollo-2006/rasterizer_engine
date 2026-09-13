#pragma once
#include <cmath>

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

    vec3 operator*(double t) const {
        return vec3(e[0] * t, e[1] * t, e[2] * t);
    }

    static double dot(const vec3& u, const vec3& v) {
        return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
    }

    static vec3 cross(const vec3& u, const vec3& v) {
        return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                    u.e[2] * v.e[0] - u.e[0] * v.e[2],
                    u.e[0] * v.e[1] - u.e[1] * v.e[0]);
    }

    vec3 normalize() const {
        double length = std::sqrt(e[0]*e[0] + e[1]*e[1] + e[2]*e[2]);
        return length == 0 ? vec3() : vec3(e[0]/length, e[1]/length, e[2]/length);
    }
};

inline vec3 operator*(double t, const vec3& v) {
    return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}
