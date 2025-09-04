#include "rt/Beam.hpp"
#include <algorithm>
#include <cmath>

namespace rt
{
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double r, double len,
           double intensity, int oid, int mid, double s, double total)
    : path(origin, dir.normalized()), radius(r), length(len), start(s),
      total_length(total < 0 ? len : total), light_intensity(intensity)
{
  object_id = oid;
  material_id = mid;
}

bool Beam::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  bool hit_any = false;
  double closest = tmax;
  Vec3 centers[3] = {path.orig, path.at(length * 0.5), path.at(length)};

  for (const Vec3 &center : centers)
  {
    Vec3 oc = r.orig - center;
    double a = Vec3::dot(r.dir, r.dir);
    double half_b = Vec3::dot(oc, r.dir);
    double c = Vec3::dot(oc, oc) - radius * radius;
    double disc = half_b * half_b - a * c;
    if (disc < 0.0)
      continue;
    double sqrt_d = std::sqrt(disc);
    double t = (-half_b - sqrt_d) / a;
    if (t < tmin || t > closest)
    {
      t = (-half_b + sqrt_d) / a;
      if (t < tmin || t > closest)
        continue;
    }

    Vec3 p = r.at(t);
    double tc = Vec3::dot(p - path.orig, path.dir);
    if (tc < 0.0 || tc > length)
      continue;

    Vec3 outward = (p - center).normalized();
    rec.t = t;
    rec.p = p;
    rec.object_id = object_id;
    rec.material_id = material_id;
    rec.beam_ratio = (start + tc) / total_length;
    rec.set_face_normal(r, outward);
    hit_any = true;
    closest = t;
  }

  return hit_any;
}

bool Beam::bounding_box(AABB &out) const
{
  Vec3 centers[3] = {path.orig, path.at(length * 0.5), path.at(length)};
  Vec3 ex(radius, radius, radius);
  Vec3 min(1e30, 1e30, 1e30);
  Vec3 max(-1e30, -1e30, -1e30);
  for (const Vec3 &c : centers)
  {
    Vec3 cmin = c - ex;
    Vec3 cmax = c + ex;
    min.x = std::min(min.x, cmin.x);
    min.y = std::min(min.y, cmin.y);
    min.z = std::min(min.z, cmin.z);
    max.x = std::max(max.x, cmax.x);
    max.y = std::max(max.y, cmax.y);
    max.z = std::max(max.z, cmax.z);
  }
  out = AABB(min, max);
  return true;
}

bool Beam::is_beam() const { return true; }

} // namespace rt
