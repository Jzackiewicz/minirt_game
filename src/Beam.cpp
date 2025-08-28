#include "rt/Beam.hpp"
#include <cmath>

namespace rt
{
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double radius_, double length_,
           int oid, int mid, double start)
    : ray(origin, dir.normalized()), radius(radius_), length(length_),
      object_id(oid), material_id(mid), start_ratio(start)
{
}

bool Beam::is_beam() const { return true; }

bool Beam::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  bool hit_any = false;
  double closest = tmax;
  Vec3 axis = ray.dir.normalized();
  Vec3 oc = r.orig - ray.orig;
  double d_dot_a = Vec3::dot(r.dir, axis);
  double oc_dot_a = Vec3::dot(oc, axis);
  Vec3 d_perp = r.dir - d_dot_a * axis;
  Vec3 oc_perp = oc - oc_dot_a * axis;
  double A = Vec3::dot(d_perp, d_perp);
  double B = 2 * Vec3::dot(d_perp, oc_perp);
  double C = Vec3::dot(oc_perp, oc_perp) - radius * radius;
  double disc = B * B - 4 * A * C;
  if (disc >= 0 && A != 0)
  {
    double sqrtD = std::sqrt(disc);
    double roots[2] = {(-B - sqrtD) / (2 * A), (-B + sqrtD) / (2 * A)};
    for (double root : roots)
    {
      if (root < tmin || root > closest)
        continue;
      double s = oc_dot_a + root * d_dot_a;
      if (s < 0 || s > length)
        continue;
      Vec3 p = r.at(root);
      Vec3 proj = ray.orig + axis * s;
      Vec3 outward = (p - proj).normalized();
      rec.t = root;
      rec.p = p;
      rec.object_id = object_id;
      rec.material_id = material_id;
      double ratio = s / length;
      rec.beam_ratio = start_ratio + ratio * (1.0 - start_ratio);
      rec.set_face_normal(r, outward);
      closest = root;
      hit_any = true;
    }
  }
  Vec3 top_center = ray.orig + axis * length;
  Vec3 bottom_center = ray.orig;
  double denom_top = Vec3::dot(r.dir, axis);
  if (std::fabs(denom_top) > 1e-9)
  {
    double t = Vec3::dot(top_center - r.orig, axis) / denom_top;
    if (t >= tmin && t <= closest)
    {
      Vec3 p = r.at(t);
      if ((p - top_center).length_squared() <= radius * radius)
      {
        rec.t = t;
        rec.p = p;
        rec.object_id = object_id;
        rec.material_id = material_id;
        rec.beam_ratio = start_ratio + (1.0 * (1.0 - start_ratio));
        rec.set_face_normal(r, axis);
        closest = t;
        hit_any = true;
      }
    }
  }
  double denom_bot = Vec3::dot(r.dir, (-1) * axis);
  if (std::fabs(denom_bot) > 1e-9)
  {
    double t = Vec3::dot(bottom_center - r.orig, (-1) * axis) / denom_bot;
    if (t >= tmin && t <= closest)
    {
      Vec3 p = r.at(t);
      if ((p - bottom_center).length_squared() <= radius * radius)
      {
        rec.t = t;
        rec.p = p;
        rec.object_id = object_id;
        rec.material_id = material_id;
        rec.beam_ratio = start_ratio;
        rec.set_face_normal(r, (-1) * axis);
        closest = t;
        hit_any = true;
      }
    }
  }
  return hit_any;
}

bool Beam::bounding_box(AABB &out) const
{
  Vec3 axis = ray.dir.normalized();
  Vec3 center = ray.orig + axis * (length * 0.5);
  Vec3 ax = axis * (length / 2);
  Vec3 ex(radius, radius, radius);
  Vec3 min = center - ax - ex;
  Vec3 max = center + ax + ex;
  out = AABB(min, max);
  return true;
}

} // namespace rt
