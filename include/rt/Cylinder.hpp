#pragma once
#include "Hittable.hpp"
#include <cmath>

namespace rt
{
struct Cylinder : public Hittable
{
  Vec3 center;
  Vec3 axis;
  double radius;
  double height;
  int object_id;
  int material_id;

  Cylinder(const Vec3 &c, const Vec3 &axis_, double r, double h, int oid,
           int mid);

  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  void translate(const Vec3 &delta) override { center += delta; }
  void rotate(const Vec3 &axis, double angle) override;
  void set_id(int id) override { object_id = id; }
  int get_id() const override { return object_id; }
};

} // namespace rt
