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
  virtual ~Hittable() = default;
  virtual bool hit(const Ray &r, double tmin, double tmax,
                   HitRecord &rec) const = 0;
  virtual bool bounding_box(AABB &out) const = 0;
  virtual bool is_beam() const { return false; }

  // --- editing support ---
  // Whether the object can be manipulated in edit mode
  bool movable = false;
  // Highlighted objects blink in inverted colour when entering edit mode
  bool highlighted = false;
  // Selected objects are being actively moved; they render with a checkered
  // pattern between their base and inverted colours.
  bool selected = false;

  // Translate the object in world space.  Default implementation is a no-op.
  virtual void translate(const Vec3 &delta) {(void)delta;}
  // Rotate object around global yaw/pitch. Default is no-op.
  virtual void rotate(double yaw, double pitch)
  {
    (void)yaw;
    (void)pitch;
  }
};

using HittablePtr = std::shared_ptr<Hittable>;

} // namespace rt
