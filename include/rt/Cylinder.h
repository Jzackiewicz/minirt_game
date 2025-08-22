#pragma once
#include "Hittable.h"
#include <cmath>

namespace rt {

struct Cylinder : public Hittable {
    Vec3 center;
    Vec3 axis;   // musi być znormalizowany
    double radius;
    double height;
    int object_id;
    int material_id;

    Cylinder(const Vec3& c, const Vec3& axis_, double r, double h,
             int oid, int mid)
        : center(c), axis(axis_.normalized()), radius(r), height(h),
          object_id(oid), material_id(mid) {}

    bool hit(const Ray& r, double tmin, double tmax, HitRecord& rec) const override {
        // przekształcamy promień do układu osi cylindra
        Vec3 oc = r.orig - center;
        double d_dot_a = Vec3::dot(r.dir, axis);
        double oc_dot_a = Vec3::dot(oc, axis);

        Vec3 d_perp = r.dir - d_dot_a * axis;
        Vec3 oc_perp = oc - oc_dot_a * axis;

        double A = Vec3::dot(d_perp, d_perp);
        double B = 2 * Vec3::dot(d_perp, oc_perp);
        double C = Vec3::dot(oc_perp, oc_perp) - radius*radius;

        double disc = B*B - 4*A*C;
        if (disc < 0) return false;

        double sqrtD = std::sqrt(disc);
        double root = (-B - sqrtD) / (2*A);
        if (root < tmin || root > tmax) {
            root = (-B + sqrtD) / (2*A);
            if (root < tmin || root > tmax) return false;
        }

        double s = oc_dot_a + root*d_dot_a;
        if (s < -height/2 || s > height/2) {
            return false; // poza zakresem wysokości
        }

        rec.t = root;
        rec.p = r.at(root);
        rec.object_id = object_id;
        rec.material_id = material_id;

        Vec3 proj = center + axis*s;
        Vec3 outward = (rec.p - proj).normalized();
        rec.set_face_normal(r, outward);
        return true;
    }

    bool bounding_box(AABB& out) const override {
        // prosta, konserwatywna skrzynka: center ± (radius,height/2)
        Vec3 ax = axis * (height/2);
        Vec3 ex(radius, radius, radius);
        Vec3 min = center - ax - ex;
        Vec3 max = center + ax + ex;
        out = AABB(min, max);
        return true;
    }
};

} // namespace rt
