
#pragma once
#include "Hittable.hpp"
#include <cmath>

namespace rt
{
struct Sphere : public Hittable
{
  Vec3 center;
  double radius;
  int object_id;
  int material_id;

  Sphere(const Vec3 &c, double r, int oid, int mid);

  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
};

} // namespace rt
