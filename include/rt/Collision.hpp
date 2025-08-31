#pragma once
#include "Hittable.hpp"
#include "Plane.hpp"
#include "Sphere.hpp"
#include "Cube.hpp"

namespace rt {
// Generic GJK-based convex intersection
bool gjk_intersect(const Hittable &a, const Hittable &b);

// Analytic narrow-phase helpers
bool sphere_sphere(const Sphere &a, const Sphere &b);

// Narrow phase between two objects (analytic or GJK fallback)
bool narrow_phase(const Hittable &a, const Hittable &b);

// Plane vs convex object
bool plane_convex(const Plane &pl, const Hittable &obj);
}
