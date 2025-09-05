
#pragma once
#include "Ray.hpp"
#include "Vec3.hpp"
#include <cmath>

class Camera
{
	public:
	Vec3 origin;
	Vec3 forward;
	Vec3 right;
	Vec3 up;
	double fov_deg;
	double aspect;

	Camera(const Vec3 &pos, const Vec3 &look_at, double fov_deg_,
		   double aspect_);

	void move(const Vec3 &delta);
	void rotate(double yaw, double pitch);
	Ray ray_through(double u, double v) const;
};
