
#pragma once
#include "Hittable.h"

namespace rt {

struct Plane : public Hittable {
    Vec3 point;
    Vec3 normal;
    int object_id;
    int material_id;

    Plane(const Vec3& p, const Vec3& n, int oid, int mid)
        : point(p), normal(n.normalized()), object_id(oid), material_id(mid) {}

    bool hit(const Ray& r, double tmin, double tmax, HitRecord& rec) const override {
        double denom = Vec3::dot(normal, r.dir);
        if (std::abs(denom) < 1e-8) return false;
        double t = Vec3::dot(point - r.orig, normal) / denom;
        if (t < tmin || t > tmax) return false;
        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, normal);
        rec.material_id = material_id;
        rec.object_id = object_id;
        return true;
    }

    bool bounding_box(AABB& out) const override {
        // Planes are infinite; give a huge box for BVH usage in small scenes (not ideal).
        double big = 1e6;
        out = AABB(Vec3(-big, -big, -big), Vec3(big, big, big));
        return true;
    }
};

} // namespace rt
