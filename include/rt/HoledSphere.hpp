#pragma once
#include "Sphere.hpp"

namespace rt
{
struct HoledSphere : public Sphere
{
  Vec3 axis;
  double hole_radius;
  HoledSphere(const Vec3 &c, double r, int oid, int mid,
              const Vec3 &dir, double hole_r);
  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
  void rotate(const Vec3 &axis, double angle) override;
};
} // namespace rt
