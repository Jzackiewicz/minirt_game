#pragma once
#include "Hittable.hpp"
#include <cmath>

namespace rt
{
struct Cylinder : public Hittable
{
  Vec3 center;
  Vec3 axis;
  double radius;
  double height;
  Cylinder(const Vec3 &c, const Vec3 &axis_, double r, double h, int oid,
           int mid);

  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  void translate(const Vec3 &delta) override { center += delta; }
  void rotate(const Vec3 &axis, double angle) override;
  Vec3 support(const Vec3 &dir) const override
  {
    double axial = Vec3::dot(dir, axis);
    Vec3 radial = dir - axial * axis;
    Vec3 p = center + axis * (axial >= 0 ? height * 0.5 : -height * 0.5);
    if (radial.length_squared() > 1e-9)
      p += radial.normalized() * radius;
    return p;
  }
};

} // namespace rt
