#include "rt/HoledSphere.hpp"
#include <cmath>

namespace rt
{
HoledSphere::HoledSphere(const Vec3 &c, double r, int oid, int mid,
                         const Vec3 &dir, double hole_r)
    : Sphere(c, r, oid, mid), axis(dir.normalized()), hole_radius(hole_r)
{
}

bool HoledSphere::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  Vec3 oc = r.orig - center;
  double a = Vec3::dot(r.dir, r.dir);
  double half_b = Vec3::dot(oc, r.dir);
  double c = Vec3::dot(oc, oc) - radius * radius;
  double discriminant = half_b * half_b - a * c;
  if (discriminant < 0)
    return false;
  double sqrtd = std::sqrt(discriminant);
  double roots[2] = {(-half_b - sqrtd) / a, (-half_b + sqrtd) / a};
  for (double root : roots)
  {
    if (root < tmin || root > tmax)
      continue;
    Vec3 p = r.at(root);
    Vec3 v = p - center;
    Vec3 perp = v - axis * Vec3::dot(v, axis);
    if (perp.length_squared() <= hole_radius * hole_radius)
      continue;
    rec.t = root;
    rec.p = p;
    rec.material_id = material_id;
    rec.object_id = object_id;
    Vec3 outward = v / radius;
    rec.set_face_normal(r, outward);
    return true;
  }
  return false;
}

void HoledSphere::rotate(const Vec3 &ax, double angle)
{
  double c = std::cos(angle);
  double s = std::sin(angle);
  axis = axis * c + Vec3::cross(ax, axis) * s + ax * Vec3::dot(ax, axis) * (1 - c);
  axis = axis.normalized();
}
} // namespace rt
