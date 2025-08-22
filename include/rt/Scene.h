
#pragma once
#include "Hittable.h"
#include "BVH.h"
#include "light.h"
#include <vector>
#include <memory>

namespace rt {

struct Scene {
    std::vector<HittablePtr> objects;
    std::vector<PointLight> lights;
    Ambient ambient{Vec3(1,1,1), 0.0};
    std::shared_ptr<Hittable> accel;

    void build_bvh() {
        if (objects.empty()) { accel.reset(); return; }
        std::vector<HittablePtr> objs = objects; // copy since builder reorders
        accel = std::make_shared<BVHNode>(objs, 0, objs.size());
    }

    bool hit(const Ray& r, double tmin, double tmax, HitRecord& rec) const {
        if (accel) return accel->hit(r, tmin, tmax, rec);
        bool hit_any = false;
        HitRecord tmp;
        double closest = tmax;
        for (auto& o : objects) {
            if (o->hit(r, tmin, closest, tmp)) {
                hit_any = true;
                closest = tmp.t;
                rec = tmp;
            }
        }
        return hit_any;
    }
};

} // namespace rt
