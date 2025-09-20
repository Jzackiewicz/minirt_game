#pragma once
#include "Cone.hpp"
#include "Laser.hpp"
#include "LightRay.hpp"
#include "Sphere.hpp"
#include <memory>

class BeamSource : public Sphere
{
        public:
        Sphere mid;
        Sphere inner;
        Cone cone;
       std::shared_ptr<Laser> beam;
       std::shared_ptr<LightRay> light;
       BeamSource(const Vec3 &c, const Vec3 &dir,
                          const std::shared_ptr<Laser> &bm,
                          const std::shared_ptr<LightRay> &lt,
                          double mid_radius, int oid, int mat_big,
                          int mat_mid, int mat_small);
        bool hit(const Ray &r, double tmin, double tmax,
                         HitRecord &rec) const override;
        bool bounding_box(AABB &out) const override;
        void translate(const Vec3 &delta) override;
        void rotate(const Vec3 &axis, double angle) override;
        Vec3 spot_direction() const override;
        bool blocks_when_transparent() const override { return true; }
        bool casts_shadow() const override { return false; }
};
