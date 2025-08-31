#include "rt/Cube.hpp"
#include <algorithm>
#include <cmath>

namespace rt
{
Cube::Cube(const Vec3 &c, double a, int oid, int mid) : center(c), half(a / 2.0)
{
  axis[0] = Vec3(1, 0, 0);
  axis[1] = Vec3(0, 1, 0);
  axis[2] = Vec3(0, 0, 1);
  object_id = oid;
  material_id = mid;
}

bool Cube::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  Vec3 oc = r.orig - center;
  double orig[3] = {Vec3::dot(oc, axis[0]), Vec3::dot(oc, axis[1]),
                    Vec3::dot(oc, axis[2])};
  double dir[3] = {Vec3::dot(r.dir, axis[0]), Vec3::dot(r.dir, axis[1]),
                   Vec3::dot(r.dir, axis[2])};

  double tmin_local = tmin;
  double tmax_local = tmax;
  Vec3 normal_local;
  for (int i = 0; i < 3; ++i)
  {
    double invD = 1.0 / dir[i];
    double t0 = (-half - orig[i]) * invD;
    double t1 = (half - orig[i]) * invD;
    Vec3 n0 = i == 0 ? Vec3(-1, 0, 0)
                     : (i == 1 ? Vec3(0, -1, 0) : Vec3(0, 0, -1));
    Vec3 n1 = i == 0 ? Vec3(1, 0, 0)
                     : (i == 1 ? Vec3(0, 1, 0) : Vec3(0, 0, 1));
    if (invD < 0.0)
    {
      std::swap(t0, t1);
      std::swap(n0, n1);
    }
    if (t0 > tmin_local)
    {
      tmin_local = t0;
      normal_local = n0;
    }
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

  Vec3 normal_world = normal_local.x * axis[0] +
                      normal_local.y * axis[1] +
                      normal_local.z * axis[2];
  rec.set_face_normal(r, normal_world);
  return true;
}

bool Cube::bounding_box(AABB &out) const
{
  Vec3 u = axis[0] * half;
  Vec3 v = axis[1] * half;
  Vec3 w = axis[2] * half;
  Vec3 corners[8] = {center - u - v - w, center - u - v + w,
                     center - u + v - w, center - u + v + w,
                     center + u - v - w, center + u - v + w,
                     center + u + v - w, center + u + v + w};
  Vec3 min = corners[0];
  Vec3 max = corners[0];
  for (int i = 1; i < 8; ++i)
  {
    min.x = std::min(min.x, corners[i].x);
    min.y = std::min(min.y, corners[i].y);
    min.z = std::min(min.z, corners[i].z);
    max.x = std::max(max.x, corners[i].x);
    max.y = std::max(max.y, corners[i].y);
    max.z = std::max(max.z, corners[i].z);
  }
  out = AABB(min, max);
  return true;
}

Vec3 Cube::support(const Vec3 &dir) const
{
  Vec3 result = center;
  for (int i = 0; i < 3; ++i)
  {
    double sign = Vec3::dot(axis[i], dir) >= 0.0 ? 1.0 : -1.0;
    result += axis[i] * (half * sign);
  }
  return result;
}

void Cube::rotate(const Vec3 &ax, double angle)
{
  auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double ang) {
    double c = std::cos(ang);
    double s = std::sin(ang);
    return v * c + Vec3::cross(axis, v) * s +
           axis * Vec3::dot(axis, v) * (1 - c);
  };
  axis[0] = rotate_vec(axis[0], ax, angle).normalized();
  axis[1] = rotate_vec(axis[1], ax, angle).normalized();
  axis[2] = rotate_vec(axis[2], ax, angle).normalized();
}

} // namespace rt
