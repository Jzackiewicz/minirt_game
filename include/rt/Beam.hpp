#pragma once
#include "Hittable.hpp"
#include "Ray.hpp"

namespace rt
{
struct Beam : public Hittable
{
  Ray ray;
  double radius;
  double length;
  int object_id;
  int material_id;
  double start_ratio;

  Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
       int oid, int mid, double start = 0.0);

  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  bool is_beam() const override;
  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
};

} // namespace rt
