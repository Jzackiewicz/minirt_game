
#pragma once
#include "Hittable.hpp"

class Plane : public Hittable
{
        public:
        Vec3 point;
        Vec3 normal;
        Vec3 u_axis;
        Vec3 v_axis;
        Plane(const Vec3 &p, const Vec3 &n, int oid, int mid);

	bool hit(const Ray &r, double tmin, double tmax,
			 HitRecord &rec) const override;
	bool bounding_box(AABB &out) const override;
	bool is_plane() const override { return true; }
	void translate(const Vec3 &delta) override { point += delta; }
	void rotate(const Vec3 &axis, double angle) override;
	ShapeType shape_type() const override { return ShapeType::Plane; }
};
