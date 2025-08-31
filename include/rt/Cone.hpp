#pragma once
#include "Hittable.hpp"
#include <cmath>

namespace rt
{
struct Cone : public Hittable
{
  Vec3 center;
  Vec3 axis;
  double radius;
  double height;
  Cone(const Vec3 &c, const Vec3 &ax, double r, double h, int oid, int mid);

  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  void translate(const Vec3 &delta) override { center += delta; }
  void rotate(const Vec3 &axis, double angle) override;
  Vec3 support(const Vec3 &dir) const override
  {
    Vec3 apex = center + axis * (height * 0.5);
    Vec3 base = center - axis * (height * 0.5);
    double axial = Vec3::dot(dir, axis);
    Vec3 radial = dir - axial * axis;
    double radial_len = radial.length();
    double k = radius / height;
    if (axial > radial_len * k)
      return apex;
    Vec3 p = base;
    if (radial_len > 1e-9)
      p += radial.normalized() * radius;
    return p;
  }
};

} // namespace rt
