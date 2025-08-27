#include "rt/Beam.hpp"

namespace rt
{
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
           int oid, int mid)
    : Cylinder(origin + dir.normalized() * (length * 0.5), dir, radius, length,
               oid, mid)
{
}

bool Beam::is_beam() const { return true; }

} // namespace rt
