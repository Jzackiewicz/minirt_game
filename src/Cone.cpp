#include "Cone.hpp"
#include <algorithm>
#include <cmath>

namespace
{
void cone_basis(const Vec3 &axis, Vec3 &u, Vec3 &v)
{
        Vec3 helper = std::fabs(axis.x) > 0.9 ? Vec3(0.0, 1.0, 0.0) : Vec3(1.0, 0.0, 0.0);
        u = Vec3::cross(helper, axis).normalized();
        if (u.length_squared() <= 1e-12)
        {
                helper = Vec3(0.0, 0.0, 1.0);
                u = Vec3::cross(helper, axis).normalized();
        }
        v = Vec3::cross(axis, u);
}
}

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
        Vec3 tex_u_axis;
        Vec3 tex_v_axis;
        cone_basis(axis, tex_u_axis, tex_v_axis);
	double k = radius / height;

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
                        rec.t = root;
                        rec.p = p;
                        rec.object_id = object_id;
                        rec.material_id = material_id;
                        rec.set_face_normal(r, normal);
                        Vec3 axis_point = apex + down * ax_dist;
                        Vec3 radial_vec = p - axis_point;
                        Vec3 radial = radial_vec.length_squared() > 1e-12
                                                ? radial_vec.normalized()
                                                : tex_u_axis;
                        double angle = std::atan2(Vec3::dot(radial, tex_v_axis),
                                                  Vec3::dot(radial, tex_u_axis));
                        double tex_u = angle / (2.0 * M_PI);
                        if (tex_u < 0.0)
                                tex_u += 1.0;
                        double tex_v = ax_dist / height;
                        tex_v = std::clamp(tex_v, 0.0, 1.0);
                        rec.u = tex_u;
                        rec.v = tex_v;
                        rec.has_uv = true;
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
                        Vec3 diff = p - base_center;
                        if (diff.length_squared() <= radius * radius)
                        {
                                rec.t = t;
                                rec.p = p;
                                rec.object_id = object_id;
                                rec.material_id = material_id;
                                rec.set_face_normal(r, (-1) * axis);
                                double tex_u = Vec3::dot(diff, tex_u_axis) / (radius * 2.0) + 0.5;
                                double tex_v = Vec3::dot(diff, tex_v_axis) / (radius * 2.0) + 0.5;
                                rec.u = tex_u;
                                rec.v = tex_v;
                                rec.has_uv = true;
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
