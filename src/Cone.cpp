#include "Cone.hpp"
#include <algorithm>
#include <cmath>

namespace
{

void build_basis(const Vec3 &axis, Vec3 &tangent, Vec3 &bitangent)
{
        Vec3 helper = (std::fabs(axis.z) < 0.999) ? Vec3(0, 0, 1) : Vec3(0, 1, 0);
        tangent = Vec3::cross(helper, axis);
        double len = tangent.length();
        if (len <= 1e-12)
        {
                helper = Vec3(1, 0, 0);
                tangent = Vec3::cross(helper, axis);
                len = tangent.length();
        }
        tangent = (len > 1e-12) ? tangent / len : Vec3(1, 0, 0);
        bitangent = Vec3::cross(axis, tangent).normalized();
}

double wrap_angle(double angle)
{
        constexpr double kTwoPi = 6.28318530717958647692;
        while (angle < 0.0)
                angle += kTwoPi;
        while (angle >= kTwoPi)
                angle -= kTwoPi;
        return angle;
}

} // namespace

Cone::Cone(const Vec3 &c, const Vec3 &ax, double r, double h, int oid, int mid)
        : center(c), axis(ax.normalized()), radius(r), height(h)
{
        object_id = oid;
        material_id = mid;
        build_basis(axis, tangent, bitangent);
}

bool Cone::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
        bool hit_any = false;
        double closest = tmax;

        auto assign_hit = [&](double t, const Vec3 &point, const Vec3 &normal,
                              double u, double v)
        {
                rec.t = t;
                rec.p = point;
                rec.object_id = object_id;
                rec.material_id = material_id;
                rec.u = std::clamp(u, 0.0, 1.0);
                rec.v = std::clamp(v, 0.0, 1.0);
                rec.set_face_normal(r, normal);
                closest = t;
                hit_any = true;
        };

        Vec3 apex = center + axis * (height * 0.5);
        Vec3 down = (-1) * axis;
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
                        Vec3 local = p - center;
                        double axial = Vec3::dot(local, axis);
                        Vec3 radial = local - axis * axial;
                        double x = Vec3::dot(radial, tangent);
                        double z = Vec3::dot(radial, bitangent);
                        double angle = std::atan2(z, x);
                        angle = wrap_angle(angle);
                        constexpr double kTwoPi = 6.28318530717958647692;
                        double u = angle / kTwoPi;
                        double v = (axial + height * 0.5) / height;
                        assign_hit(root, p, normal, u, v);
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
                                Vec3 diff = p - base_center;
                                double u = Vec3::dot(diff, tangent) / (2.0 * radius) + 0.5;
                                double v = Vec3::dot(diff, bitangent) / (2.0 * radius) + 0.5;
                                v = 1.0 - v;
                                assign_hit(t, p, (-1) * axis, u, v);
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
        tangent = rotate_vec(tangent, ax, angle).normalized();
        bitangent = Vec3::cross(axis, tangent).normalized();
}
