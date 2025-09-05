#pragma once
#include "Hittable.hpp"

class Cube : public Hittable
{
	public:
	Vec3 center;
	Vec3 half; // half dimensions along local axes (length, width, height)
	// Local orthonormal basis representing cube orientation
	Vec3 axis[3];

	Cube(const Vec3 &c, const Vec3 &orientation, double L, double W, double H,
		 int oid, int mid);

	bool hit(const Ray &r, double tmin, double tmax,
			 HitRecord &rec) const override;
	bool bounding_box(AABB &out) const override;
	void translate(const Vec3 &delta) override { center += delta; }
	void rotate(const Vec3 &axis, double angle) override;
	ShapeType shape_type() const override { return ShapeType::Cube; }
};
