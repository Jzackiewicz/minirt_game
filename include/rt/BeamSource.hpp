#pragma once
#include "Sphere.hpp"
#include <memory>

namespace rt {
struct Beam;
struct BeamSource : public Sphere {
  Sphere mid;
  Sphere inner;
  std::shared_ptr<Beam> beam;
  BeamSource(const Vec3 &c, double base_radius, const std::shared_ptr<Beam> &bm,
             int oid, int mat_outer, int mat_mid, int mat_inner);
  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override { return Sphere::bounding_box(out); }
  void translate(const Vec3 &delta) override;
  void rotate(const Vec3 &axis, double angle) override;
};
} // namespace rt
