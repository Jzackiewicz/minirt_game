
#pragma once
#include "Ray.hpp"
#include "Vec3.hpp"
#include <algorithm>

class AABB
{
	public:
	Vec3 min;
	Vec3 max;

	AABB();
	AABB(const Vec3 &a, const Vec3 &b);

	bool hit(const Ray &r, double tmin, double tmax) const;
	bool intersects(const AABB &other) const;
	bool intersects_plane(const Vec3 &point, const Vec3 &normal) const;
	static AABB surrounding_box(const AABB &box0, const AABB &box1);
};
