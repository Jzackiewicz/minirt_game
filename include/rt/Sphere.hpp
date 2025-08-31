
#pragma once
#include "Hittable.hpp"
#include <cmath>

namespace rt
{
struct Sphere : public Hittable
{
  Vec3 center;
  double radius;
  Sphere(const Vec3 &c, double r, int oid, int mid);

  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  void translate(const Vec3 &delta) override { center += delta; }
  Vec3 support(const Vec3 &dir) const override;
  bool is_sphere() const override { return true; }
};

} // namespace rt
