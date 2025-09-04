#include "rt/BeamSource.hpp"
#include "rt/Beam.hpp"
#include <cmath>

namespace rt {
BeamSource::BeamSource(const Vec3 &c, double base_radius,
                       const std::shared_ptr<Beam> &bm, int oid, int mat_outer,
                       int mat_mid, int mat_inner)
    : Sphere(c, base_radius * 16.0 / 9.0, oid, mat_outer),
      mid(c, base_radius * 4.0 / 3.0, oid, mat_mid),
      inner(c, base_radius, oid, mat_inner), beam(bm) {
  casts_shadow = false;
  mid.casts_shadow = false;
  inner.casts_shadow = false;
  mid.collidable = false;
  inner.collidable = false;
}

bool BeamSource::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const {
  bool hit_any = false;
  HitRecord tmp;
  double closest = tmax;
  if (Sphere::hit(r, tmin, closest, tmp)) {
    hit_any = true;
    closest = tmp.t;
    rec = tmp;
  }
  if (mid.hit(r, tmin, closest, tmp)) {
    hit_any = true;
    closest = tmp.t;
    rec = tmp;
    rec.material_id = mid.material_id;
  }
  if (inner.hit(r, tmin, closest, tmp)) {
    hit_any = true;
    closest = tmp.t;
    rec = tmp;
    rec.material_id = inner.material_id;
  }
  if (hit_any)
    rec.object_id = object_id;
  return hit_any;
}

void BeamSource::translate(const Vec3 &delta) {
  Sphere::translate(delta);
  mid.translate(delta);
  inner.translate(delta);
  if (beam)
    beam->path.orig += delta;
}

void BeamSource::rotate(const Vec3 &ax, double angle) {
  auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double ang) {
    double c = std::cos(ang);
    double s = std::sin(ang);
    return v * c + Vec3::cross(axis, v) * s + axis * Vec3::dot(axis, v) * (1 - c);
  };
  if (beam)
    beam->path.dir = rotate_vec(beam->path.dir, ax, angle).normalized();
}

} // namespace rt
