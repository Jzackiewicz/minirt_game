#include "Plane.hpp"
#include <cmath>

namespace
{
void build_plane_basis(const Vec3 &normal, Vec3 &tangent, Vec3 &bitangent)
{
        Vec3 helper = std::fabs(normal.x) > 0.9 ? Vec3(0.0, 1.0, 0.0) : Vec3(1.0, 0.0, 0.0);
        tangent = Vec3::cross(helper, normal).normalized();
        if (tangent.length_squared() <= 1e-12)
        {
                helper = Vec3(0.0, 0.0, 1.0);
                tangent = Vec3::cross(helper, normal).normalized();
        }
        bitangent = Vec3::cross(normal, tangent).normalized();
}
}

Plane::Plane(const Vec3 &p, const Vec3 &n, int oid, int mid)
        : point(p), normal(n.normalized())
{
        object_id = oid;
        material_id = mid;
        build_plane_basis(normal, tangent, bitangent);
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
        Vec3 diff = rec.p - point;
        double u = Vec3::dot(diff, tangent);
        double v = Vec3::dot(diff, bitangent);
        rec.u = u - std::floor(u);
        rec.v = v - std::floor(v);
        rec.has_uv = true;
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
        tangent = rotate_vec(tangent, axis, angle).normalized();
        bitangent = Vec3::cross(normal, tangent).normalized();
}
