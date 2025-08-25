
#pragma once
#include "Vec3.h"
#include "Ray.h"
#include "AABB.h"
#include <memory>

namespace rt {

struct Material; // forward

struct HitRecord {
    Vec3 p;
    Vec3 normal;
    double t;
    int object_id;
    int material_id;
    bool front_face;
    double beam_ratio = 0.0;
    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = Vec3::dot(r.dir, outward_normal) < 0;
        normal = front_face ? outward_normal : outward_normal * -1.0;
    }
};

struct Hittable {
    virtual ~Hittable() = default;
    virtual bool hit(const Ray& r, double tmin, double tmax, HitRecord& rec) const = 0;
    virtual bool bounding_box(AABB& out) const = 0;
    virtual bool is_beam() const { return false; }
};

using HittablePtr = std::shared_ptr<Hittable>;

} // namespace rt
