#include "Cylinder.hpp"
#include <algorithm>
#include <cmath>

namespace
{

void make_cylinder_basis(const Vec3 &axis, Vec3 &tangent, Vec3 &bitangent)
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

Cylinder::Cylinder(const Vec3 &c, const Vec3 &axis_, double r, double h,
				   int oid, int mid)
	: center(c), axis(axis_.normalized()), radius(r), height(h)
{
	object_id = oid;
	material_id = mid;
}

bool Cylinder::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
	bool hit_any = false;
	double closest = tmax;

	Vec3 tangent, bitangent;
	make_cylinder_basis(axis, tangent, bitangent);

	Vec3 oc = r.orig - center;
	double d_dot_a = Vec3::dot(r.dir, axis);
	double oc_dot_a = Vec3::dot(oc, axis);

	Vec3 d_perp = r.dir - d_dot_a * axis;
	Vec3 oc_perp = oc - oc_dot_a * axis;

	double A = Vec3::dot(d_perp, d_perp);
	double B = 2 * Vec3::dot(d_perp, oc_perp);
	double C = Vec3::dot(oc_perp, oc_perp) - radius * radius;

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
			double s = oc_dot_a + root * d_dot_a;
			if (s < -height / 2 || s > height / 2)
			{
				continue;
			}
			Vec3 p = r.at(root);
			Vec3 proj = center + axis * s;
			Vec3 radial = p - proj;
			Vec3 outward = radial.normalized();
			double angle = std::atan2(Vec3::dot(outward, bitangent),
			                                  Vec3::dot(outward, tangent));
			rec.u = wrap_unit(angle / (2.0 * M_PI));
			double v = (s + height / 2.0) / height;
			rec.v = std::clamp(v, 0.0, 1.0);
			rec.has_uv = true;
			rec.t = root;
			rec.p = p;
			rec.object_id = object_id;
			rec.material_id = material_id;
			rec.beam_ratio = (s + height / 2) / height;
			rec.set_face_normal(r, outward);
			closest = root;
			hit_any = true;
		}
	}

	Vec3 top_center = center + axis * (height / 2);
	Vec3 bottom_center = center - axis * (height / 2);

	double denom_top = Vec3::dot(r.dir, axis);
	if (std::fabs(denom_top) > 1e-9)
	{
		double t = Vec3::dot(top_center - r.orig, axis) / denom_top;
		if (t >= tmin && t <= closest)
		{
			Vec3 p = r.at(t);
			if ((p - top_center).length_squared() <= radius * radius)
			{
				rec.t = t;
				rec.p = p;
				rec.object_id = object_id;
				rec.material_id = material_id;
				rec.beam_ratio = 1.0;
				Vec3 rel = p - top_center;
				double u = Vec3::dot(rel, tangent) / radius;
				double v = Vec3::dot(rel, bitangent) / radius;
				rec.u = (u + 1.0) * 0.5;
				rec.v = (v + 1.0) * 0.5;
				rec.has_uv = true;
				rec.set_face_normal(r, axis);
				closest = t;
				hit_any = true;
			}
		}
	}

	double denom_bot = Vec3::dot(r.dir, (-1) * axis);
	if (std::fabs(denom_bot) > 1e-9)
	{
		double t = Vec3::dot(bottom_center - r.orig, (-1) * axis) / denom_bot;
		if (t >= tmin && t <= closest)
		{
			Vec3 p = r.at(t);
			if ((p - bottom_center).length_squared() <= radius * radius)
			{
				rec.t = t;
				rec.p = p;
				rec.object_id = object_id;
				rec.material_id = material_id;
				rec.beam_ratio = 0.0;
				Vec3 rel = p - bottom_center;
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

bool Cylinder::bounding_box(AABB &out) const
{
	Vec3 ax = axis * (height / 2);
	Vec3 abs_ax(std::fabs(ax.x), std::fabs(ax.y), std::fabs(ax.z));
	Vec3 ex(radius, radius, radius);
	Vec3 min = center - abs_ax - ex;
	Vec3 max = center + abs_ax + ex;
	out = AABB(min, max);
	return true;
}

void Cylinder::rotate(const Vec3 &ax, double angle)
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
