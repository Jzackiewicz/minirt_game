
#pragma once
#include "Hittable.hpp"
#include <cmath>

class Sphere : public Hittable
{
	public:
	Vec3 center;
	double radius;
	Sphere(const Vec3 &c, double r, int oid, int mid);

	bool hit(const Ray &r, double tmin, double tmax,
			 HitRecord &rec) const override;
	bool bounding_box(AABB &out) const override;
	void translate(const Vec3 &delta) override { center += delta; }
	ShapeType shape_type() const override { return ShapeType::Sphere; }
};
