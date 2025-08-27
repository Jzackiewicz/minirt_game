#include "rt/AABB.hpp"
#include <algorithm>

namespace rt
{
AABB::AABB() : min(0, 0, 0), max(0, 0, 0) {}

AABB::AABB(const Vec3 &a, const Vec3 &b) : min(a), max(b) {}

bool AABB::hit(const Ray &r, double tmin, double tmax) const
{
  for (int a = 0; a < 3; ++a)
  {
    double invD = 1.0 / (a == 0 ? r.dir.x : (a == 1 ? r.dir.y : r.dir.z));
    double t0 = ((a == 0 ? min.x : (a == 1 ? min.y : min.z)) -
                 (a == 0 ? r.orig.x : (a == 1 ? r.orig.y : r.orig.z))) *
                invD;
    double t1 = ((a == 0 ? max.x : (a == 1 ? max.y : max.z)) -
                 (a == 0 ? r.orig.x : (a == 1 ? r.orig.y : r.orig.z))) *
                invD;
    if (invD < 0.0)
    {
      std::swap(t0, t1);
    }
    tmin = t0 > tmin ? t0 : tmin;
    tmax = t1 < tmax ? t1 : tmax;
    if (tmax <= tmin)
    {
      return false;
    }
  }
  return true;
}

AABB AABB::surrounding_box(const AABB &box0, const AABB &box1)
{
  Vec3 small(std::min(box0.min.x, box1.min.x), std::min(box0.min.y, box1.min.y),
             std::min(box0.min.z, box1.min.z));
  Vec3 big(std::max(box0.max.x, box1.max.x), std::max(box0.max.y, box1.max.y),
           std::max(box0.max.z, box1.max.z));
  return AABB(small, big);
}

} // namespace rt
