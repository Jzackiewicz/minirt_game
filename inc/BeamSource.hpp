#pragma once
#include "Laser.hpp"
#include "Sphere.hpp"
#include <memory>

namespace rt
{
struct BeamSource : public Sphere
{
  Sphere mid;
  Sphere inner;
  std::shared_ptr<Laser> beam;
  BeamSource(const Vec3 &c, const Vec3 &dir, const std::shared_ptr<Laser> &bm,
             double radius, int oid, int mat_big, int mat_mid, int mat_small);
  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override { return Sphere::bounding_box(out); }
  void translate(const Vec3 &delta) override;
  void rotate(const Vec3 &axis, double angle) override;
  Vec3 spot_direction() const override;
};
} // namespace rt
