#include "rt/Beam.hpp"
#include <algorithm>
#include <cmath>

namespace rt
{
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double r, double len, int oid,
           int mid, double s, double total)
    : path(origin, dir.normalized()), radius(r), length(len), start(s),
      total_length(total < 0 ? len : total)
{
  object_id = oid;
  material_id = mid;
}

bool Beam::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  Vec3 v = path.dir;
  Vec3 dp = r.dir - v * Vec3::dot(r.dir, v);
  double a = Vec3::dot(dp, dp);
  if (a < 1e-12)
    return false;

  Vec3 ao = r.orig - path.orig;
  Vec3 ao_perp = ao - v * Vec3::dot(ao, v);
  double half_b = Vec3::dot(dp, ao_perp);
  double c = Vec3::dot(ao_perp, ao_perp) - radius * radius;
  double discriminant = half_b * half_b - a * c;
  if (discriminant < 0.0)
    return false;

  double sqrtd = std::sqrt(discriminant);
  double root = (-half_b - sqrtd) / a;
  if (root < tmin || root > tmax)
  {
    root = (-half_b + sqrtd) / a;
    if (root < tmin || root > tmax)
      return false;
  }

  Vec3 p = r.at(root);
  double proj = Vec3::dot(p - path.orig, v);
  if (proj < 0.0 || proj > length)
    return false;

  Vec3 center = path.at(proj);
  Vec3 outward = p - center;
  if (outward.length_squared() < 1e-12)
  {
    outward = Vec3::cross(v, Vec3(1, 0, 0));
    if (outward.length_squared() < 1e-12)
      outward = Vec3::cross(v, Vec3(0, 1, 0));
    outward = outward.normalized();
  }
  else
  {
    outward = outward.normalized();
  }

  rec.t = root;
  rec.p = p;
  rec.object_id = object_id;
  rec.material_id = material_id;
  rec.beam_ratio = (start + proj) / total_length;
  rec.set_face_normal(r, outward);
  return true;
}

bool Beam::bounding_box(AABB &out) const
{
  Vec3 start = path.orig;
  Vec3 end = path.at(length);
  Vec3 min(std::min(start.x, end.x), std::min(start.y, end.y),
           std::min(start.z, end.z));
  Vec3 max(std::max(start.x, end.x), std::max(start.y, end.y),
           std::max(start.z, end.z));
  Vec3 ex(radius, radius, radius);
  out = AABB(min - ex, max + ex);
  return true;
}

bool Beam::is_beam() const { return true; }

} // namespace rt
