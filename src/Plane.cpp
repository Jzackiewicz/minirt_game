#include "Plane.hpp"
#include <cmath>

namespace rt
{
Plane::Plane(const Vec3 &p, const Vec3 &n, int oid, int mid)
    : point(p), normal(n.normalized())
{
  object_id = oid;
  material_id = mid;
}

bool Plane::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  double denom = Vec3::dot(normal, r.dir);
  if (std::abs(denom) < 1e-8)
  {
    return false;
  }
  double t = Vec3::dot(point - r.orig, normal) / denom;
  if (t < tmin || t > tmax)
  {
    return false;
  }
  rec.t = t;
  rec.p = r.at(t);
  rec.set_face_normal(r, normal);
  rec.material_id = material_id;
  rec.object_id = object_id;
  return true;
}

bool Plane::bounding_box(AABB &out) const
{
  (void)out;
  return false;
}

void Plane::rotate(const Vec3 &axis, double angle)
{
  auto rotate_vec = [](const Vec3 &v, const Vec3 &ax, double ang)
  {
    double c = std::cos(ang);
    double s = std::sin(ang);
    return v * c + Vec3::cross(ax, v) * s + ax * Vec3::dot(ax, v) * (1 - c);
  };
  normal = rotate_vec(normal, axis, angle).normalized();
}

} // namespace rt
