#include "Ray.hpp"

Ray::Ray() {}

Ray::Ray(const Vec3 &o, const Vec3 &d) : orig(o), dir(d) {}

Vec3 Ray::at(double t) const { return orig + dir * t; }
