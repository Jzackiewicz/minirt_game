#pragma once
#include "Ray.hpp"

class LightRay
{
	public:
	Ray ray;
	double intensity;
	LightRay(const Vec3 &origin, const Vec3 &dir, double intens)
		: ray(origin, dir.normalized()), intensity(intens)
	{
	}
};
