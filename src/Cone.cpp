#include "rt/Cone.hpp"
#include <cmath>

namespace rt
{
Cone::Cone(const Vec3 &c, const Vec3 &ax, double r, double h, int oid, int mid)
    : center(c), axis(ax.normalized()), radius(r), height(h)
{
  object_id = oid;
  material_id = mid;
}

bool Cone::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  bool hit_any = false;
  double closest = tmax;

  Vec3 apex = center + axis * (height * 0.5);
  Vec3 down = (-1) * axis;
  double k = radius / height;

  Vec3 oc = r.orig - apex;
  double oc_dot_d = Vec3::dot(oc, down);
  double d_dot_d = Vec3::dot(r.dir, down);

  Vec3 oc_perp = oc - oc_dot_d * down;
  Vec3 d_perp = r.dir - d_dot_d * down;

  double A = Vec3::dot(d_perp, d_perp) - k * k * d_dot_d * d_dot_d;
  double B = 2 * Vec3::dot(d_perp, oc_perp) - 2 * k * k * d_dot_d * oc_dot_d;
  double C = Vec3::dot(oc_perp, oc_perp) - k * k * oc_dot_d * oc_dot_d;

  double disc = B * B - 4 * A * C;
  if (disc >= 0)
  {
    double sqrtD = std::sqrt(disc);
    double roots[2] = {(-B - sqrtD) / (2 * A), (-B + sqrtD) / (2 * A)};
    for (double root : roots)
    {
      if (root < tmin || root > closest)
      {
        continue;
      }
      double y = oc_dot_d + root * d_dot_d;
      if (y < 0 || y > height)
      {
        continue;
      }
      Vec3 p = r.at(root);
      double ax_dist = y;
      Vec3 x_parallel = down * ax_dist;
      Vec3 x_perp = (oc + root * r.dir) - x_parallel;
      Vec3 normal = (x_perp - (k * k * ax_dist) * down).normalized();
      rec.t = root;
      rec.p = p;
      rec.object_id = object_id;
      rec.material_id = material_id;
      rec.set_face_normal(r, normal);
      closest = root;
      hit_any = true;
    }
  }

  Vec3 base_center = center - axis * (height * 0.5);
  double denom = Vec3::dot(r.dir, (-1) * axis);
  if (std::fabs(denom) > 1e-9)
  {
    double t = Vec3::dot(base_center - r.orig, (-1) * axis) / denom;
    if (t >= tmin && t <= closest)
    {
      Vec3 p = r.at(t);
      if ((p - base_center).length_squared() <= radius * radius)
      {
        rec.t = t;
        rec.p = p;
        rec.object_id = object_id;
        rec.material_id = material_id;
        rec.set_face_normal(r, (-1) * axis);
        closest = t;
        hit_any = true;
      }
    }
  }

  return hit_any;
}

bool Cone::bounding_box(AABB &out) const
{
  Vec3 ax = axis * (height * 0.5);
  Vec3 ex(radius, radius, radius);
  Vec3 min = center - ax - ex;
  Vec3 max = center + ax + ex;
  out = AABB(min, max);
  return true;
}

void Cone::rotate(const Vec3 &ax, double angle)
{
  auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double ang)
  {
    double c = std::cos(ang);
    double s = std::sin(ang);
    return v * c + Vec3::cross(axis, v) * s + axis * Vec3::dot(axis, v) * (1 - c);
  };
  axis = rotate_vec(axis, ax, angle).normalized();
}

} // namespace rt
