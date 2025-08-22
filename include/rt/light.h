
#pragma once
#include "Vec3.h"
#include <vector>

namespace rt {

struct PointLight {
    Vec3 position;
    Vec3 color; // 0..1
    double intensity; // 0..1
    PointLight(const Vec3& p, const Vec3& c, double i) : position(p), color(c), intensity(i) {}
};

struct Ambient {
    Vec3 color;
    double intensity;
    Ambient(const Vec3& c, double i) : color(c), intensity(i) {}
};

} // namespace rt
