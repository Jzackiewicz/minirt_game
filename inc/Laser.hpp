#pragma once
#include "Hittable.hpp"
#include "Ray.hpp"
#include <memory>

class Laser : public Hittable
{
        public:
       Ray path;
       const double radius;
       double length;
        double start;
        double total_length;
        double light_intensity;
        Vec3 color;
        std::weak_ptr<Hittable> source;
       Laser(const Vec3 &origin, const Vec3 &dir, double length, double intensity,
                 int oid, int mid, double start = 0.0, double total = -1.0);

	bool hit(const Ray &r, double tmin, double tmax,
			 HitRecord &rec) const override;
	bool bounding_box(AABB &out) const override;
	bool is_beam() const override;
	ShapeType shape_type() const override { return ShapeType::Beam; }
	Vec3 spot_direction() const override { return path.dir; }
};
