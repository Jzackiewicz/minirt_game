#include "rt/HoledSphere.hpp"
#include <cmath>

namespace rt
{
HoledSphere::HoledSphere(const Vec3 &c, const Vec3 &d, double r, int oid, int mid)
    : Sphere(c, r, oid, mid), dir(d.normalized()), hole_radius(r * 0.25)
{
}

bool HoledSphere::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  if (!Sphere::hit(r, tmin, tmax, rec))
    return false;
  Vec3 local = (rec.p - center) / radius;
  double cos_cutoff = std::sqrt(1.0 - (hole_radius / radius) * (hole_radius / radius));
  if (Vec3::dot(local, dir) >= cos_cutoff)
    return false;
  return true;
}

void HoledSphere::rotate(const Vec3 &ax, double angle)
{
  auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double ang) {
    double c = std::cos(ang);
    double s = std::sin(ang);
    return v * c + Vec3::cross(axis, v) * s + axis * Vec3::dot(axis, v) * (1 - c);
  };
  dir = rotate_vec(dir, ax, angle).normalized();
}
} // namespace rt
