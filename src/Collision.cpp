#include "rt/Collision.hpp"
#include <array>

namespace rt {

static bool same_direction(const Vec3 &a, const Vec3 &b) {
  return Vec3::dot(a, b) > 0;
}

static bool handle_simplex(std::array<Vec3, 4> &simplex, int &size, Vec3 &dir) {
  if (size == 2) {
    Vec3 A = simplex[1];
    Vec3 B = simplex[0];
    Vec3 AO = (-1.0) * A;
    Vec3 AB = B - A;
    if (same_direction(AB, AO)) {
      dir = Vec3::cross(Vec3::cross(AB, AO), AB);
    } else {
      simplex[0] = A;
      size = 1;
      dir = AO;
    }
    return false;
  } else if (size == 3) {
    Vec3 A = simplex[2];
    Vec3 B = simplex[1];
    Vec3 C = simplex[0];
    Vec3 AO = (-1.0) * A;
    Vec3 AB = B - A;
    Vec3 AC = C - A;
    Vec3 ABC = Vec3::cross(AB, AC);

    Vec3 AB_perp = Vec3::cross(ABC, AB);
    if (same_direction(AB_perp, AO)) {
      simplex[0] = B;
      simplex[1] = A;
      size = 2;
      dir = Vec3::cross(Vec3::cross(AB, AO), AB);
      return false;
    }
    Vec3 AC_perp = Vec3::cross(AC, ABC);
    if (same_direction(AC_perp, AO)) {
      simplex[1] = A;
      simplex[0] = C;
      size = 2;
      dir = Vec3::cross(Vec3::cross(AC, AO), AC);
      return false;
    }
    if (same_direction(ABC, AO)) {
      dir = ABC;
    } else {
      std::swap(simplex[0], simplex[1]);
      dir = (-1.0) * ABC;
    }
    return false;
  } else if (size == 4) {
    Vec3 A = simplex[3];
    Vec3 B = simplex[2];
    Vec3 C = simplex[1];
    Vec3 D = simplex[0];
    Vec3 AO = (-1.0) * A;
    Vec3 AB = B - A;
    Vec3 AC = C - A;
    Vec3 AD = D - A;
    Vec3 ABC = Vec3::cross(AB, AC);
    Vec3 ACD = Vec3::cross(AC, AD);
    Vec3 ADB = Vec3::cross(AD, AB);
    if (same_direction(ABC, AO)) {
      simplex[0] = C;
      simplex[1] = B;
      simplex[2] = A;
      size = 3;
      dir = ABC;
      return false;
    }
    if (same_direction(ACD, AO)) {
      simplex[0] = D;
      simplex[1] = C;
      simplex[2] = A;
      size = 3;
      dir = ACD;
      return false;
    }
    if (same_direction(ADB, AO)) {
      simplex[0] = B;
      simplex[1] = D;
      simplex[2] = A;
      size = 3;
      dir = ADB;
      return false;
    }
    return true; // origin is inside
  }
  return false;
}

bool gjk_intersect(const Hittable &a, const Hittable &b) {
  auto support = [&](const Vec3 &d) {
    return a.support(d) - b.support((-1.0) * d);
  };

  std::array<Vec3, 4> simplex;
  int size = 0;
  Vec3 dir(1, 0, 0);
  simplex[size++] = support(dir);
  dir = (-1.0) * simplex[0];

  for (int iter = 0; iter < 32; ++iter) {
    Vec3 p = support(dir);
    if (Vec3::dot(p, dir) <= 0)
      return false;
    simplex[size++] = p;
    if (handle_simplex(simplex, size, dir))
      return true;
  }
  return false;
}

bool sphere_sphere(const Sphere &a, const Sphere &b) {
  double r = a.radius + b.radius;
  return (a.center - b.center).length_squared() <= r * r;
}

bool narrow_phase(const Hittable &a, const Hittable &b) {
  if (a.is_sphere() && b.is_sphere())
    return sphere_sphere(static_cast<const Sphere &>(a),
                         static_cast<const Sphere &>(b));
  return gjk_intersect(a, b);
}

bool plane_convex(const Plane &pl, const Hittable &obj) {
  Vec3 p1 = obj.support(pl.normal);
  Vec3 p2 = obj.support((-1.0) * pl.normal);
  double d1 = Vec3::dot(p1 - pl.point, pl.normal);
  double d2 = Vec3::dot(p2 - pl.point, pl.normal);
  return d1 * d2 <= 0;
}

} // namespace rt
