#include "rt/BeamSource.hpp"
#include <cmath>

namespace rt
{
BeamSource::BeamSource(const Vec3 &c, const Vec3 &dir, int oid, int mat_big,
                       int mat_mid, int mat_small)
    : Sphere(c, 0.6, oid, mat_big),
      mid(c, 0.6 * 0.67, -oid - 1, mat_mid),
      inner(c, 0.6 * 0.33, -oid - 2, mat_small), direction(dir)
{}

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
    Vec3 beam_dir = direction;
    Vec3 to_hit = (tmp.p - inner.center).normalized();
    const double hole_cos = std::sqrt(1.0 - 0.25 * 0.25);
    if (Vec3::dot(beam_dir, to_hit) < hole_cos)
    {
      hit_any = true;
      closest = tmp.t;
      rec = tmp;
    }
  }
  return hit_any;
}

void BeamSource::translate(const Vec3 &delta)
{
  Sphere::translate(delta);
  mid.translate(delta);
  inner.translate(delta);
}

void BeamSource::rotate(const Vec3 &ax, double angle)
{
  auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double ang) {
    double c = std::cos(ang);
    double s = std::sin(ang);
    return v * c + Vec3::cross(axis, v) * s + axis * Vec3::dot(axis, v) * (1 - c);
  };
  direction = rotate_vec(direction, ax, angle).normalized();
}

Vec3 BeamSource::spot_direction() const
{
  return direction;
}

} // namespace rt
