#include "rt/Beam.hpp"

namespace rt
{
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
           int oid, int mid, double start)
    : Cylinder(origin + dir.normalized() * (length * 0.5), dir, radius, length,
               oid, mid),
      start_ratio(start)
{
}

bool Beam::is_beam() const { return true; }

bool Beam::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  if (!Cylinder::hit(r, tmin, tmax, rec))
    return false;
  rec.beam_ratio = start_ratio + rec.beam_ratio * (1.0 - start_ratio);
  return true;
}

} // namespace rt
