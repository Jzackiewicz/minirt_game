#pragma once
#include "Hittable.hpp"
#include "Ray.hpp"
#include <memory>

namespace rt
{
struct Beam : public Hittable
{
  Ray path;
  double radius;
  double length;
  double start;
  double total_length;
  double intensity;
  std::weak_ptr<Hittable> source;
  Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
       int oid, int mid, double start = 0.0, double total = -1.0);

  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  bool is_beam() const override;
  ShapeType shape_type() const override { return ShapeType::Beam; }
  Vec3 spot_direction() const override { return path.dir; }
};

} // namespace rt
