
#pragma once
#include "Vec3.h"

namespace rt {
struct Ray {
    Vec3 orig;
    Vec3 dir;
    Ray() {}
    Ray(const Vec3& o, const Vec3& d) : orig(o), dir(d) {}
    Vec3 at(double t) const { return orig + dir * t; }
};
} // namespace rt
