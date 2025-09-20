#include "Plane.hpp"
#include <cmath>

namespace
{

void make_plane_basis(const Vec3 &normal, Vec3 &u, Vec3 &v)
{
	Vec3 n = normal.normalized();
	Vec3 helper = (std::fabs(n.x) > 0.9) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
	u = Vec3::cross(helper, n);
	double len = u.length();
	if (len <= 1e-12)
	{
		helper = Vec3(0, 0, 1);
		u = Vec3::cross(helper, n);
		len = u.length();
	}
	if (len <= 1e-12)
		u = Vec3(1, 0, 0);
	else
		u = u / len;
	v = Vec3::cross(n, u);
}

} // namespace


Plane::Plane(const Vec3 &p, const Vec3 &n, int oid, int mid)
	: point(p), normal(n.normalized())
{
	object_id = oid;
	material_id = mid;
}

bool Plane::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
	double denom = Vec3::dot(normal, r.dir);
	if (std::abs(denom) < 1e-8)
	{
		return false;
	}
	double t = Vec3::dot(point - r.orig, normal) / denom;
	if (t < tmin || t > tmax)
	{
		return false;
	}
	rec.t = t;
	rec.p = r.at(t);
	Vec3 u_axis, v_axis;
	make_plane_basis(normal, u_axis, v_axis);
	Vec3 rel = rec.p - point;
	rec.u = Vec3::dot(rel, u_axis);
	rec.v = Vec3::dot(rel, v_axis);
	rec.has_uv = true;
	rec.set_face_normal(r, normal);
	rec.material_id = material_id;
	rec.object_id = object_id;
	return true;
}

bool Plane::bounding_box(AABB &out) const
{
	(void)out;
	return false;
}

void Plane::rotate(const Vec3 &axis, double angle)
{
	auto rotate_vec = [](const Vec3 &v, const Vec3 &ax, double ang)
	{
		double c = std::cos(ang);
		double s = std::sin(ang);
		return v * c + Vec3::cross(ax, v) * s + ax * Vec3::dot(ax, v) * (1 - c);
	};
	normal = rotate_vec(normal, axis, angle).normalized();
}
