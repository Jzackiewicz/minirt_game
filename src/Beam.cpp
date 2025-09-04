#include "rt/Beam.hpp"

namespace rt
{
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double r, double len,
           double intensity, int oid, int mid, double s, double total)
    : path(origin, dir.normalized()), radius(r), length(len), start(s),
      total_length(total < 0 ? len : total), light_intensity(intensity)
{
  object_id = oid;
  material_id = mid;
}

bool Beam::hit(const Ray &, double, double, HitRecord &) const
{
  return false;
}

bool Beam::bounding_box(AABB &) const
{
  return false;
}

bool Beam::is_beam() const { return true; }

} // namespace rt
