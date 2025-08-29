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
  Vec3 u = r.dir;
  Vec3 v = path.dir;
  Vec3 w0 = r.orig - path.orig;
  double a = Vec3::dot(u, u);
  double b = Vec3::dot(u, v);
  double c = Vec3::dot(v, v);
  double d = Vec3::dot(u, w0);
  double e = Vec3::dot(v, w0);
  double denom = a * c - b * b;

  double sc, tc;
  if (std::fabs(denom) < 1e-9)
  {
    sc = -d / a;
    tc = (a * e - b * d) / (a * c);
  }
  else
  {
    sc = (b * e - c * d) / denom;
    tc = (a * e - b * d) / denom;
  }

  if (sc < tmin || sc > tmax)
    return false;
  if (tc < 0.0 || tc > length)
    return false;

  Vec3 pr = r.at(sc);
  Vec3 pb = path.at(tc);
  Vec3 diff = pr - pb;
  double dist2 = diff.length_squared();
  if (dist2 > radius * radius)
    return false;

  Vec3 outward;
  if (dist2 > 1e-12)
  {
    outward = diff.normalized();
  }
  else
  {
    outward = Vec3::cross(path.dir, Vec3(1, 0, 0));
    if (outward.length_squared() < 1e-12)
      outward = Vec3::cross(path.dir, Vec3(0, 1, 0));
    outward = outward.normalized();
  }

  rec.t = sc;
  rec.p = pr;
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
