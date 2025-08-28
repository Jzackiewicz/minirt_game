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
  rec.beam_ratio = tc / length;
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
