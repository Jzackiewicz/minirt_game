#pragma once
#include "Hittable.hpp"

namespace rt
{
struct Cube : public Hittable
{
  Vec3 center;
  double half;
  // Local orthonormal basis representing cube orientation
  Vec3 axis[3];

  Cube(const Vec3 &c, double a, int oid, int mid);

  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  void translate(const Vec3 &delta) override { center += delta; }
  void rotate(const Vec3 &axis, double angle) override;
  Vec3 support(const Vec3 &dir) const override
  {
    Vec3 local(Vec3::dot(dir, axis[0]), Vec3::dot(dir, axis[1]),
               Vec3::dot(dir, axis[2]));
    Vec3 corner(local.x >= 0 ? half : -half,
                local.y >= 0 ? half : -half,
                local.z >= 0 ? half : -half);
    return center + axis[0] * corner.x + axis[1] * corner.y + axis[2] * corner.z;
  }
};
} // namespace rt
