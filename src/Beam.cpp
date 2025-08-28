#include "rt/Beam.hpp"
#include <algorithm>
#include <cmath>

namespace rt
{
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double r, double len, int oid,
           int mid, double s, double total)
    : path(origin, dir.normalized()), radius(r), length(len), start(s),
      total_length(total < 0 ? len : total), object_id(oid), material_id(mid)
{
}

bool Beam::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  // Intersection with a finite cylinder aligned with `path`
  Vec3 v = path.dir;            // cylinder axis (normalized)
  Vec3 dp = r.orig - path.orig; // vector from cylinder base to ray origin

  double dp_par = Vec3::dot(dp, v);
  Vec3 dp_perp = dp - v * dp_par;
  double dir_par = Vec3::dot(r.dir, v);
  Vec3 dir_perp = r.dir - v * dir_par;

  double a = Vec3::dot(dir_perp, dir_perp);
  double b = 2.0 * Vec3::dot(dir_perp, dp_perp);
  double c = Vec3::dot(dp_perp, dp_perp) - radius * radius;

  if (std::fabs(a) < 1e-12)
    return false; // Ray parallel to cylinder axis

  double det = b * b - 4.0 * a * c;
  if (det < 0.0)
    return false;

  double sqrt_det = std::sqrt(det);
  double t0 = (-b - sqrt_det) / (2.0 * a);
  double t1 = (-b + sqrt_det) / (2.0 * a);
  if (t0 > t1)
    std::swap(t0, t1);

  double t = t0;
  double tc = dp_par + dir_par * t;
  if (t < tmin || t > tmax || tc < 0.0 || tc > length)
  {
    t = t1;
    tc = dp_par + dir_par * t;
    if (t < tmin || t > tmax || tc < 0.0 || tc > length)
      return false;
  }

  Vec3 p_hit = r.at(t);
  Vec3 axis_pt = path.at(tc);
  Vec3 outward = (p_hit - axis_pt).normalized();

  rec.t = t;
  rec.p = p_hit;
  rec.object_id = object_id;
  rec.material_id = material_id;
  rec.beam_ratio = (start + tc) / total_length;
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
