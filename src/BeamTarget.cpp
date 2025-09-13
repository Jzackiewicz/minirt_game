#include "BeamTarget.hpp"

BeamTarget::BeamTarget(const Vec3 &c, double outer_radius, int oid, int mat_big, int mat_mid, int mat_small)
    : Sphere(c, outer_radius, oid, mat_big),
      mid(c, outer_radius * 0.75, -oid - 1, mat_mid),
      inner(c, outer_radius * 0.5, -oid - 2, mat_small) {}

bool BeamTarget::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const {
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
    }
    if (inner.hit(r, tmin, closest, tmp)) {
        hit_any = true;
        closest = tmp.t;
        rec = tmp;
    }
    return hit_any;
}

void BeamTarget::translate(const Vec3 &delta) {
    Sphere::translate(delta);
    mid.translate(delta);
    inner.translate(delta);
}
