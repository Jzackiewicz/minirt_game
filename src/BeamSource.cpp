#include "rt/BeamSource.hpp"
#include <cmath>

namespace rt
{
BeamSource::BeamSource(const Vec3 &c, const Vec3 &dir,
                       const std::shared_ptr<Beam> &bm, int oid,
                       int mat_big, int mat_mid, int mat_small,
                       PointLight *lt)
    : Sphere(c, 0.6, oid, mat_big),
      mid(c, 0.6 * 0.67, oid, mat_mid),
      inner(c, 0.6 * 0.33, oid, mat_small), beam(bm), light(lt)
{
  if (light)
  {
    light->position = c + dir.normalized() * 0.6;
    light->ignore_id = oid;
  }
}

bool BeamSource::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  bool hit_any = false;
  HitRecord tmp;
  double closest = tmax;
  if (Sphere::hit(r, tmin, closest, tmp))
  {
    hit_any = true;
    closest = tmp.t;
    rec = tmp;
  }
  if (mid.hit(r, tmin, closest, tmp))
  {
    hit_any = true;
    closest = tmp.t;
    rec = tmp;
  }
  if (inner.hit(r, tmin, closest, tmp))
  {
    hit_any = true;
    closest = tmp.t;
    rec = tmp;
  }
  if (hit_any)
    rec.object_id = object_id;
  return hit_any;
}

void BeamSource::translate(const Vec3 &delta)
{
  Sphere::translate(delta);
  mid.translate(delta);
  inner.translate(delta);
  if (beam)
    beam->path.orig += delta;
  if (light)
    light->position += delta;
}

void BeamSource::rotate(const Vec3 &ax, double angle)
{
  auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double ang) {
    double c = std::cos(ang);
    double s = std::sin(ang);
    return v * c + Vec3::cross(axis, v) * s + axis * Vec3::dot(axis, v) * (1 - c);
  };
  if (beam)
    beam->path.dir = rotate_vec(beam->path.dir, ax, angle).normalized();
  if (light)
    light->position = center + beam->path.dir.normalized() * 0.6;
}

} // namespace rt
