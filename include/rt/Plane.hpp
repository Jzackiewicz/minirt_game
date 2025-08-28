#pragma once
#include "Hittable.hpp"
#include <cmath>

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
  void rotate(double yaw, double pitch) override
  {
    auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double angle) {
      double c = std::cos(angle);
      double s = std::sin(angle);
      return v * c + Vec3::cross(axis, v) * s + axis * Vec3::dot(axis, v) * (1 - c);
    };
    Vec3 world_up(0, 1, 0);
    normal = rotate_vec(normal, world_up, yaw);
    Vec3 right = Vec3::cross(world_up, normal).normalized();
    normal = rotate_vec(normal, right, pitch);
    normal = normal.normalized();
  }
};

} // namespace rt
