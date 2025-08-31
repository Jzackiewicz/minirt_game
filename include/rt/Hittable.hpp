
#pragma once
#include "AABB.hpp"
#include "Ray.hpp"
#include "Vec3.hpp"
#include <memory>

namespace rt
{

struct Material;

struct HitRecord
{
  Vec3 p;
  Vec3 normal;
  double t;
  int object_id;
  int material_id;
  bool front_face;
  double beam_ratio = 0.0;
  void set_face_normal(const Ray &r, const Vec3 &outward_normal);
};

struct Hittable
{
  bool movable = false;
  int object_id = 0;
  int material_id = 0;
  virtual ~Hittable() = default;
  virtual bool hit(const Ray &r, double tmin, double tmax,
                   HitRecord &rec) const = 0;
  virtual bool bounding_box(AABB &out) const = 0;
  virtual bool is_beam() const { return false; }
  virtual bool is_plane() const { return false; }
  virtual bool is_bvh() const { return false; }
  virtual bool is_sphere() const { return false; }
  // Support mapping for convex collision detection
  // Returns the furthest point in a given direction. Default implementation
  // returns the origin which is suitable for non-convex or analytic cases that
  // override collision checks.
  virtual Vec3 support(const Vec3 &dir) const
  {
    (void)dir;
    return Vec3();
  }
  // default translation does nothing
  virtual void translate(const Vec3 &delta) { (void)delta; }
  // default rotation does nothing
  virtual void rotate(const Vec3 &axis, double angle)
  {
    (void)axis;
    (void)angle;
  }
};

using HittablePtr = std::shared_ptr<Hittable>;

} // namespace rt
