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

void BeamTarget::start_goal() {
    if (!goal_active) {
        goal_active = true;
        goal_phase = 1;
        goal_timer = 0.0;
    }
}

static void apply_phase(int phase, std::vector<Material> &mats, const Sphere &outer,
                        const Sphere &mid, const Sphere &inner) {
    Vec3 base = mats[inner.material_id].base_color;
    if (phase == 1) {
        mats[outer.material_id].color = base * 0.5;
        mats[mid.material_id].color = base;
        mats[inner.material_id].color = Vec3(0, 0, 0);
    } else if (phase == 2) {
        mats[outer.material_id].color = base;
        mats[mid.material_id].color = Vec3(0, 0, 0);
        mats[inner.material_id].color = base * 0.5;
    } else {
        mats[outer.material_id].color = Vec3(0, 0, 0);
        mats[mid.material_id].color = base * 0.5;
        mats[inner.material_id].color = base;
    }
}

void BeamTarget::update_goal(double dt, std::vector<Material> &mats) {
    if (goal_active) {
        goal_timer += dt;
        if (goal_timer >= 0.2) {
            goal_timer = 0.0;
            goal_phase = (goal_phase + 1) % 3;
        }
        apply_phase(goal_phase, mats, *this, mid, inner);
    } else if (goal_phase != 0) {
        goal_phase = 0;
        goal_timer = 0.0;
        apply_phase(goal_phase, mats, *this, mid, inner);
    }
}
