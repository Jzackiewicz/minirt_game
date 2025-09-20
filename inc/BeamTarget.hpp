#pragma once
#include "Sphere.hpp"
#include "material.hpp"
#include <vector>

class BeamTarget : public Sphere {
public:
    Sphere mid;
    Sphere inner;
    bool goal_active = false;
    int goal_phase = 0;
    double goal_timer = 0.0;
    BeamTarget(const Vec3 &c, double outer_radius, int oid, int mat_big, int mat_mid, int mat_small);
    bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const override;
    bool bounding_box(AABB &out) const override { return Sphere::bounding_box(out); }
    void translate(const Vec3 &delta) override;
    bool blocks_when_transparent() const override { return true; }
    bool casts_shadow() const override { return goal_active; }
    ShapeType shape_type() const override { return ShapeType::BeamTarget; }
    void start_goal();
    void update_goal(double dt, std::vector<Material> &mats);
};
