#include "Cone.hpp"
#include <algorithm>
#include <cmath>

namespace
{
struct OrthonormalBasis
{
        Vec3 u;
        Vec3 v;
        Vec3 w;
};

OrthonormalBasis make_basis(const Vec3 &axis)
{
        OrthonormalBasis basis{};
        double len2 = axis.length_squared();
        if (len2 <= 1e-12)
        {
                basis.w = Vec3(0, 0, 1);
                basis.u = Vec3(1, 0, 0);
                basis.v = Vec3(0, 1, 0);
                return basis;
        }
        basis.w = axis / std::sqrt(len2);
        Vec3 helper = (std::fabs(basis.w.z) < 0.999) ? Vec3(0, 0, 1) : Vec3(0, 1, 0);
        Vec3 u = Vec3::cross(helper, basis.w);
        double ulen = u.length();
        if (ulen <= 1e-12)
        {
                helper = Vec3(0, 1, 0);
                u = Vec3::cross(helper, basis.w);
                ulen = u.length();
        }
        basis.u = (ulen > 1e-12) ? u / ulen : Vec3(1, 0, 0);
        basis.v = Vec3::cross(basis.w, basis.u);
        return basis;
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
        OrthonormalBasis basis = make_basis(axis);

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
                        Vec3 to_center = p - center;
                        double height_param = Vec3::dot(to_center, axis);
                        Vec3 radial = to_center - axis * height_param;
                        double radial_len = radial.length();
                        Vec3 radial_dir = (radial_len > 1e-12) ? radial / radial_len : basis.u;
                        double angle = std::atan2(Vec3::dot(radial_dir, basis.v), Vec3::dot(radial_dir, basis.u));
                        double tex_u = 1.0 - (angle + M_PI) / (2.0 * M_PI);
                        tex_u -= std::floor(tex_u);
                        if (tex_u < 0.0)
                                tex_u += 1.0;
                        double tex_v = (height_param + height * 0.5) / height;
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
                        if ((p - base_center).length_squared() <= radius * radius)
                        {
                                rec.t = t;
                                rec.p = p;
                                rec.object_id = object_id;
                                rec.material_id = material_id;
                                rec.set_face_normal(r, (-1) * axis);
                                double tex_u = 0.5;
                                double tex_v = 0.5;
                                if (radius > 1e-8)
                                {
                                        tex_u = 0.5 + Vec3::dot(p - base_center, basis.u) / (2.0 * radius);
                                        tex_v = 0.5 + Vec3::dot(p - base_center, basis.v) / (2.0 * radius);
                                }
                                tex_u = std::clamp(tex_u, 0.0, 1.0);
                                tex_v = std::clamp(1.0 - tex_v, 0.0, 1.0);
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
