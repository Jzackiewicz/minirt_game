#include "rt/Sphere.hpp"
#include <cmath>

namespace rt
{
Sphere::Sphere(const Vec3 &c, double r, int oid, int mid)
    : center(c), radius(r)
{
  object_id = oid;
  material_id = mid;
}

bool Sphere::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  Vec3 oc = r.orig - center;
  double a = Vec3::dot(r.dir, r.dir);
  double half_b = Vec3::dot(oc, r.dir);
  double c = Vec3::dot(oc, oc) - radius * radius;
  double discriminant = half_b * half_b - a * c;
  if (discriminant < 0)
  {
    return false;
  }
  double sqrtd = std::sqrt(discriminant);

  double root = (-half_b - sqrtd) / a;
  if (root < tmin || tmax < root)
  {
    root = (-half_b + sqrtd) / a;
    if (root < tmin || tmax < root)
    {
      return false;
    }
  }
  rec.t = root;
  rec.p = r.at(rec.t);
  rec.material_id = material_id;
  rec.object_id = object_id;
  Vec3 outward = (rec.p - center) / radius;
  rec.set_face_normal(r, outward);
  return true;
}

bool Sphere::bounding_box(AABB &out) const
{
  Vec3 rad(radius, radius, radius);
  out = AABB(center - rad, center + rad);
  return true;
}

Vec3 Sphere::support(const Vec3 &dir) const
{
  Vec3 n = dir.normalized();
  return center + n * radius;
}

} // namespace rt
