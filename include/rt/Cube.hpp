#pragma once
#include "Hittable.hpp"

namespace rt
{
struct Cube : public Hittable
{
  Vec3 center;
  double half;
  // Local orthonormal basis representing cube orientation
  Vec3 axis[3];

  Cube(const Vec3 &c, double a, int oid, int mid);

  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  void translate(const Vec3 &delta) override { center += delta; }
  void rotate(const Vec3 &axis, double angle) override;
  Vec3 support(const Vec3 &dir) const override;
  ShapeKind kind() const override { return ShapeKind::Cube; }
};
} // namespace rt
