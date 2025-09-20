#include "Sphere.hpp"
#include <algorithm>
#include <cmath>

Sphere::Sphere(const Vec3 &c, double r, int oid, int mid) : center(c), radius(r)
{
	object_id = oid;
	material_id = mid;
}

bool Sphere::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
	Vec3 oc = r.orig - center;
	double a = Vec3::dot(r.dir, r.dir);
	double half_b = Vec3::dot(oc, r.dir);
	double c = Vec3::dot(oc, oc) - radius * radius;
	double discriminant = half_b * half_b - a * c;
	if (discriminant < 0)
	{
		return false;
	}
	double sqrtd = std::sqrt(discriminant);

	double root = (-half_b - sqrtd) / a;
	if (root < tmin || tmax < root)
	{
		root = (-half_b + sqrtd) / a;
		if (root < tmin || tmax < root)
		{
			return false;
		}
	}
	rec.t = root;
        rec.p = r.at(rec.t);
        rec.material_id = material_id;
        rec.object_id = object_id;
        Vec3 outward = (rec.p - center) / radius;
        rec.set_face_normal(r, outward);
        double phi = std::atan2(outward.z, outward.x);
        double theta = std::acos(std::clamp(outward.y, -1.0, 1.0));
        rec.u = (phi + M_PI) / (2.0 * M_PI);
        rec.v = theta / M_PI;
        return true;
}

bool Sphere::bounding_box(AABB &out) const
{
	Vec3 rad(radius, radius, radius);
	out = AABB(center - rad, center + rad);
	return true;
}
