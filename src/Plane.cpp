#include "Plane.hpp"
#include <cmath>

namespace
{

Vec3 orthonormal_axis(const Vec3 &normal)
{
        Vec3 helper = (std::fabs(normal.z) < 0.999) ? Vec3(0, 0, 1) : Vec3(0, 1, 0);
        Vec3 u = Vec3::cross(helper, normal);
        double len = u.length();
        if (len <= 1e-12)
        {
                helper = Vec3(1, 0, 0);
                u = Vec3::cross(helper, normal);
                len = u.length();
        }
        return (len > 1e-12) ? u / len : Vec3(1, 0, 0);
}

}

Plane::Plane(const Vec3 &p, const Vec3 &n, int oid, int mid)
        : point(p), normal(n.normalized())
{
        object_id = oid;
        material_id = mid;
        u_axis = orthonormal_axis(normal);
        v_axis = Vec3::cross(normal, u_axis).normalized();
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
        Vec3 local = rec.p - point;
        double u = Vec3::dot(local, u_axis);
        double v = Vec3::dot(local, v_axis);
        rec.u = u - std::floor(u);
        rec.v = v - std::floor(v);
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
        u_axis = rotate_vec(u_axis, axis, angle).normalized();
        v_axis = Vec3::cross(normal, u_axis);
        double len = v_axis.length();
        if (len > 1e-12)
        {
                v_axis = v_axis / len;
        }
        else
        {
                u_axis = orthonormal_axis(normal);
                v_axis = Vec3::cross(normal, u_axis).normalized();
        }
}
