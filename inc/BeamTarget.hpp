#pragma once
#include "Sphere.hpp"

class BeamTarget : public Sphere {
public:
    Sphere mid;
    Sphere inner;
    BeamTarget(const Vec3 &c, double outer_radius, int oid, int mat_big, int mat_mid, int mat_small);
    bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
    bool bounding_box(AABB &out) const override { return Sphere::bounding_box(out); }
    void translate(const Vec3 &delta) override;
    bool blocks_when_transparent() const override { return true; }
};
