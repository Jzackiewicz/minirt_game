
#pragma once
#include "Hittable.hpp"

namespace rt
{
struct Plane : public Hittable
{
  Vec3 point;
  Vec3 normal;
  int object_id;
  int material_id;

  Plane(const Vec3 &p, const Vec3 &n, int oid, int mid);

  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  void translate(const Vec3 &delta) override { point += delta; }
  void rotate(const Vec3 &axis, double angle) override;
};

} // namespace rt
