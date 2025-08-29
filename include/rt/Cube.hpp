#pragma once
#include "Hittable.hpp"

namespace rt
{
struct Cube : public Hittable
{
  Vec3 center;
  double half;
  Cube(const Vec3 &c, double a, int oid, int mid);

  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  void translate(const Vec3 &delta) override { center += delta; }
};
} // namespace rt
