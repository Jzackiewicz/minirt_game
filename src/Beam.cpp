#include "rt/Beam.hpp"
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
  bool hit_any = false;
  double closest = tmax;

  Vec3 axis = path.dir;
  Vec3 oc = r.orig - path.orig;

  double d_dot_a = Vec3::dot(r.dir, axis);
  double oc_dot_a = Vec3::dot(oc, axis);

  Vec3 d_perp = r.dir - d_dot_a * axis;
  Vec3 oc_perp = oc - oc_dot_a * axis;

  double A = Vec3::dot(d_perp, d_perp);
  double B = 2 * Vec3::dot(d_perp, oc_perp);
  double C = Vec3::dot(oc_perp, oc_perp) - radius * radius;

  double disc = B * B - 4 * A * C;
  if (disc >= 0)
  {
    double sqrtD = std::sqrt(disc);
    double roots[2] = {(-B - sqrtD) / (2 * A), (-B + sqrtD) / (2 * A)};
    for (double root : roots)
    {
      if (root < tmin || root > closest)
        continue;
      double s = oc_dot_a + root * d_dot_a;
      if (s < 0.0 || s > length)
        continue;
      Vec3 p = r.at(root);
      Vec3 proj = path.orig + axis * s;
      Vec3 outward = (p - proj).normalized();
      rec.t = root;
      rec.p = p;
      rec.object_id = object_id;
      rec.material_id = material_id;
      rec.beam_ratio = (start + s) / total_length;
      rec.set_face_normal(r, outward);
      closest = root;
      hit_any = true;
    }
  }

  Vec3 start_center = path.orig;
  Vec3 end_center = path.at(length);

  double denom_start = Vec3::dot(r.dir, (-1) * axis);
  if (std::fabs(denom_start) > 1e-9)
  {
    double t = Vec3::dot(start_center - r.orig, (-1) * axis) / denom_start;
    if (t >= tmin && t <= closest)
    {
      Vec3 p = r.at(t);
      if ((p - start_center).length_squared() <= radius * radius)
      {
        rec.t = t;
        rec.p = p;
        rec.object_id = object_id;
        rec.material_id = material_id;
        rec.beam_ratio = start / total_length;
        rec.set_face_normal(r, (-1) * axis);
        closest = t;
        hit_any = true;
      }
    }
  }

  double denom_end = Vec3::dot(r.dir, axis);
  if (std::fabs(denom_end) > 1e-9)
  {
    double t = Vec3::dot(end_center - r.orig, axis) / denom_end;
    if (t >= tmin && t <= closest)
    {
      Vec3 p = r.at(t);
      if ((p - end_center).length_squared() <= radius * radius)
      {
        rec.t = t;
        rec.p = p;
        rec.object_id = object_id;
        rec.material_id = material_id;
        rec.beam_ratio = (start + length) / total_length;
        rec.set_face_normal(r, axis);
        closest = t;
        hit_any = true;
      }
    }
  }

  return hit_any;
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
