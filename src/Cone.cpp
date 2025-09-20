#include "Cone.hpp"
#include <algorithm>
#include <cmath>

namespace
{

void make_cone_basis(const Vec3 &axis, Vec3 &tangent, Vec3 &bitangent)
{
        Vec3 n = axis.normalized();
        if (n.length_squared() <= 1e-12)
                n = Vec3(0, 1, 0);
        Vec3 helper = (std::fabs(n.x) > 0.9) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        tangent = Vec3::cross(helper, n);
        double len = tangent.length();
        if (len <= 1e-12)
        {
                helper = Vec3(0, 0, 1);
                tangent = Vec3::cross(helper, n);
                len = tangent.length();
        }
        tangent = (len <= 1e-12) ? Vec3(1, 0, 0) : tangent / len;
        bitangent = Vec3::cross(n, tangent);
}

double wrap_unit(double value)
{
        double wrapped = value - std::floor(value);
        if (wrapped < 0.0)
                wrapped += 1.0;
        return wrapped;
}

} // namespace

Cone::Cone(const Vec3 &c, const Vec3 &ax, double r, double h, int oid, int mid)
	: center(c), axis(ax.normalized()), radius(r), height(h)
{
	object_id = oid;
	material_id = mid;
}

bool Cone::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
	bool hit_any = false;
	double closest = tmax;

	Vec3 apex = center + axis * (height * 0.5);
	Vec3 down = (-1) * axis;
	double k = radius / height;

	Vec3 tangent, bitangent;
	make_cone_basis(axis, tangent, bitangent);

	Vec3 oc = r.orig - apex;
	double oc_dot_d = Vec3::dot(oc, down);
	double d_dot_d = Vec3::dot(r.dir, down);

	Vec3 oc_perp = oc - oc_dot_d * down;
	Vec3 d_perp = r.dir - d_dot_d * down;

	double A = Vec3::dot(d_perp, d_perp) - k * k * d_dot_d * d_dot_d;
	double B = 2 * Vec3::dot(d_perp, oc_perp) - 2 * k * k * d_dot_d * oc_dot_d;
	double C = Vec3::dot(oc_perp, oc_perp) - k * k * oc_dot_d * oc_dot_d;

	double disc = B * B - 4 * A * C;
	if (disc >= 0)
	{
		double sqrtD = std::sqrt(disc);
		double roots[2] = {(-B - sqrtD) / (2 * A), (-B + sqrtD) / (2 * A)};
		for (double root : roots)
		{
			if (root < tmin || root > closest)
			{
				continue;
			}
			double y = oc_dot_d + root * d_dot_d;
			if (y < 0 || y > height)
			{
				continue;
			}
			Vec3 p = r.at(root);
			double ax_dist = y;
			Vec3 x_parallel = down * ax_dist;
			Vec3 x_perp = (oc + root * r.dir) - x_parallel;
			Vec3 normal = (x_perp - (k * k * ax_dist) * down).normalized();
			Vec3 axis_point = apex + down * y;
			Vec3 radial_dir = (p - axis_point).normalized();
			double angle = std::atan2(Vec3::dot(radial_dir, bitangent),
			                                  Vec3::dot(radial_dir, tangent));
			rec.u = wrap_unit(angle / (2.0 * M_PI));
			rec.v = std::clamp(y / height, 0.0, 1.0);
			rec.has_uv = true;
			rec.t = root;
			rec.p = p;
			rec.object_id = object_id;
			rec.material_id = material_id;
			rec.set_face_normal(r, normal);
			closest = root;
			hit_any = true;
		}
	}

        Vec3 base_center = center - axis * (height * 0.5);
        double denom = Vec3::dot(r.dir, (-1) * axis);
        if (std::fabs(denom) > 1e-9)
        {
                double t = Vec3::dot(base_center - r.orig, (-1) * axis) / denom;
                if (t >= tmin && t <= closest)
                {
                        Vec3 p = r.at(t);
                        if ((p - base_center).length_squared() <= radius * radius)
                        {
                                rec.t = t;
                                rec.p = p;
                                rec.object_id = object_id;
                                rec.material_id = material_id;
                                Vec3 rel = p - base_center;
                                double u = Vec3::dot(rel, tangent) / radius;
                                double v = Vec3::dot(rel, bitangent) / radius;
                                rec.u = (u + 1.0) * 0.5;
                                rec.v = (v + 1.0) * 0.5;
                                rec.has_uv = true;
                                rec.set_face_normal(r, (-1) * axis);
                                closest = t;
                                hit_any = true;
                        }
                }
	}

	return hit_any;
}

bool Cone::bounding_box(AABB &out) const
{
	Vec3 ax = axis * (height * 0.5);
	Vec3 abs_ax(std::fabs(ax.x), std::fabs(ax.y), std::fabs(ax.z));
	Vec3 ex(radius, radius, radius);
	Vec3 min = center - abs_ax - ex;
	Vec3 max = center + abs_ax + ex;
	out = AABB(min, max);
	return true;
}

void Cone::rotate(const Vec3 &ax, double angle)
{
	auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double ang)
	{
		double c = std::cos(ang);
		double s = std::sin(ang);
		return v * c + Vec3::cross(axis, v) * s +
			   axis * Vec3::dot(axis, v) * (1 - c);
	};
	axis = rotate_vec(axis, ax, angle).normalized();
}
