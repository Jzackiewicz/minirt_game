#include "rt/Cube.hpp"
#include <cmath>

namespace rt
{
Cube::Cube(const Vec3 &c, double a, int oid, int mid) : center(c), half(a / 2.0)
{
  object_id = oid;
  material_id = mid;
}

bool Cube::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  Vec3 min = center - Vec3(half, half, half);
  Vec3 max = center + Vec3(half, half, half);

  double tmin_local = tmin;
  double tmax_local = tmax;
  for (int axis = 0; axis < 3; ++axis)
  {
    double origin = axis == 0 ? r.orig.x : (axis == 1 ? r.orig.y : r.orig.z);
    double direction = axis == 0 ? r.dir.x : (axis == 1 ? r.dir.y : r.dir.z);
    double invD = 1.0 / direction;
    double t0 = ((axis == 0 ? min.x : (axis == 1 ? min.y : min.z)) - origin) * invD;
    double t1 = ((axis == 0 ? max.x : (axis == 1 ? max.y : max.z)) - origin) * invD;
    if (invD < 0.0)
    {
      std::swap(t0, t1);
    }
    tmin_local = t0 > tmin_local ? t0 : tmin_local;
    tmax_local = t1 < tmax_local ? t1 : tmax_local;
    if (tmax_local <= tmin_local)
    {
      return false;
    }
  }
  rec.t = tmin_local;
  rec.p = r.at(rec.t);
  rec.material_id = material_id;
  rec.object_id = object_id;

  const double eps = 1e-6;
  Vec3 outward(0, 0, 0);
  if (std::fabs(rec.p.x - min.x) < eps)
    outward = Vec3(-1, 0, 0);
  else if (std::fabs(rec.p.x - max.x) < eps)
    outward = Vec3(1, 0, 0);
  else if (std::fabs(rec.p.y - min.y) < eps)
    outward = Vec3(0, -1, 0);
  else if (std::fabs(rec.p.y - max.y) < eps)
    outward = Vec3(0, 1, 0);
  else if (std::fabs(rec.p.z - min.z) < eps)
    outward = Vec3(0, 0, -1);
  else
    outward = Vec3(0, 0, 1);

  rec.set_face_normal(r, outward);
  return true;
}

bool Cube::bounding_box(AABB &out) const
{
  Vec3 rad(half, half, half);
  out = AABB(center - rad, center + rad);
  return true;
}

} // namespace rt
