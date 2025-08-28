#pragma once
#include "Hittable.hpp"
#include <cmath>

namespace rt
{
struct Cone : public Hittable
{
  Vec3 center;
  Vec3 axis;
  double radius;
  double height;
  int object_id;
  int material_id;

  Cone(const Vec3 &c, const Vec3 &ax, double r, double h, int oid, int mid);

  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;

  void translate(const Vec3 &delta) override { center += delta; }
  void rotate(double yaw, double pitch) override
  {
    auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double angle) {
      double c = std::cos(angle);
      double s = std::sin(angle);
      return v * c + Vec3::cross(axis, v) * s + axis * Vec3::dot(axis, v) * (1 - c);
    };
    Vec3 world_up(0, 1, 0);
    axis = rotate_vec(axis, world_up, yaw);
    Vec3 right = Vec3::cross(world_up, axis).normalized();
    axis = rotate_vec(axis, right, pitch);
    axis = axis.normalized();
  }
};

} // namespace rt
