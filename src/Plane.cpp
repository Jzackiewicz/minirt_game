#include "Plane.hpp"
#include <cmath>

namespace
{

void compute_basis(const Vec3 &normal, Vec3 &u_axis, Vec3 &v_axis)
{
        Vec3 n = normal.normalized();
        Vec3 helper = (std::fabs(n.x) > 0.9) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        u_axis = Vec3::cross(n, helper);
        double len = u_axis.length();
        if (len <= 1e-8)
        {
                helper = Vec3(0, 0, 1);
                u_axis = Vec3::cross(n, helper);
                len = u_axis.length();
        }
        if (len <= 1e-8)
        {
                u_axis = Vec3(1, 0, 0);
        }
        else
        {
                u_axis = u_axis / len;
        }
        v_axis = Vec3::cross(n, u_axis);
}

double wrap_unit(double value)
{
        double wrapped = value - std::floor(value);
        if (wrapped < 0.0)
                wrapped += 1.0;
        return wrapped;
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
        rec.set_face_normal(r, normal);
        rec.material_id = material_id;
        rec.object_id = object_id;
        Vec3 u_axis;
        Vec3 v_axis;
        compute_basis(normal, u_axis, v_axis);
        Vec3 diff = rec.p - point;
        double u = Vec3::dot(diff, u_axis);
        double v = Vec3::dot(diff, v_axis);
        rec.u = wrap_unit(u);
        rec.v = wrap_unit(v);
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
