#pragma once
#include "Beam.hpp"
#include "Sphere.hpp"
#include <memory>

namespace rt
{
struct HoledSphere;
struct BeamSource : public Sphere
{
  Sphere mid;
  std::shared_ptr<HoledSphere> inner;
  std::shared_ptr<Beam> beam;
  BeamSource(const Vec3 &c, const Vec3 &dir, const std::shared_ptr<Beam> &bm,
             int oid, int mat_big, int mat_mid);
  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override { return Sphere::bounding_box(out); }
  void translate(const Vec3 &delta) override;
  void rotate(const Vec3 &axis, double angle) override;
};
} // namespace rt
