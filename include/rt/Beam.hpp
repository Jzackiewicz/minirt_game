#pragma once
#include "Cylinder.hpp"

namespace rt
{
struct Beam : public Cylinder
{
  Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
       int oid, int mid);

  bool is_beam() const override;
};

} // namespace rt
