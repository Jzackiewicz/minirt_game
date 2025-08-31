
#pragma once
#include "Ray.hpp"
#include "Vec3.hpp"
#include <algorithm>

namespace rt
{
struct AABB
{
  Vec3 min;
  Vec3 max;

  AABB();
  AABB(const Vec3 &a, const Vec3 &b);

  bool hit(const Ray &r, double tmin, double tmax) const;
  bool intersects(const AABB &other) const;
  static AABB surrounding_box(const AABB &box0, const AABB &box1);
};
} // namespace rt
