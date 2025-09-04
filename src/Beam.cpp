#include "rt/Beam.hpp"
#include <algorithm>
#include <cmath>

namespace rt
{
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double len, double intensity,
           int oid, int mid, double s, double total)
    : path(origin, dir.normalized()), length(len), start(s),
      total_length(total < 0 ? len : total), light_intensity(intensity)
{
  object_id = oid;
  material_id = mid;
}

bool Beam::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  (void)r;
  (void)tmin;
  (void)tmax;
  (void)rec;
  return false;
}

bool Beam::bounding_box(AABB &out) const
{
  Vec3 start = path.orig;
  Vec3 end = path.at(length);
  Vec3 min(std::min(start.x, end.x), std::min(start.y, end.y),
           std::min(start.z, end.z));
  Vec3 max(std::max(start.x, end.x), std::max(start.y, end.y),
           std::max(start.z, end.z));
  out = AABB(min, max);
  return true;
}

bool Beam::is_beam() const { return true; }

} // namespace rt
