#pragma once
#include "Cylinder.hpp"

namespace rt
{
struct Beam : public Cylinder
{
  double start_ratio;

  Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
       int oid, int mid, double start = 0.0);

  bool is_beam() const override;
  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
};

} // namespace rt
