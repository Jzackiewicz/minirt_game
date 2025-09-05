#include "Collision.hpp"
#include "AABB.hpp"
#include "Cone.hpp"
#include "Cube.hpp"
#include "Cylinder.hpp"
#include "Plane.hpp"
#include "Sphere.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace
{

static Vec3 support(const Sphere &s, const Vec3 &dir)
{
	Vec3 d = dir.length_squared() > 0 ? dir.normalized() : Vec3(1, 0, 0);
	return s.center + d * s.radius;
}

static Vec3 support(const Cube &c, const Vec3 &dir)
{
	Vec3 res = c.center;
	for (int i = 0; i < 3; ++i)
	{
		double sign = Vec3::dot(c.axis[i], dir) > 0 ? 1.0 : -1.0;
		double extent = (i == 0 ? c.half.x : (i == 1 ? c.half.y : c.half.z));
		res += c.axis[i] * (extent * sign);
	}
	return res;
}

static Vec3 support(const Cylinder &cy, const Vec3 &dir)
{
	double axial = Vec3::dot(dir, cy.axis);
	Vec3 res =
		cy.center + cy.axis * (axial > 0 ? cy.height * 0.5 : -cy.height * 0.5);
	Vec3 radial = dir - cy.axis * axial;
	if (radial.length_squared() > 1e-9)
		res += radial.normalized() * cy.radius;
	return res;
}

static Vec3 support(const Cone &co, const Vec3 &dir)
{
	double axial = Vec3::dot(dir, co.axis);
	Vec3 apex = co.center + co.axis * (co.height * 0.5);
	Vec3 base = co.center - co.axis * (co.height * 0.5);
	if (axial > 0)
		return apex;
	Vec3 radial = dir - co.axis * axial;
	if (radial.length_squared() > 1e-9)
		radial = radial.normalized() * co.radius;
	return base + radial;
}

static Vec3 support_box(const AABB &b, const Vec3 &dir)
{
	return Vec3(dir.x > 0 ? b.max.x : b.min.x, dir.y > 0 ? b.max.y : b.min.y,
				dir.z > 0 ? b.max.z : b.min.z);
}

static bool sat_cube_cube(const Cube &a, const Cube &b)
{
	constexpr double EPS = 1e-6;

	double R[3][3];
	double absR[3][3];
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			R[i][j] = Vec3::dot(a.axis[i], b.axis[j]);
			absR[i][j] = std::fabs(R[i][j]) + EPS;
		}
	}

	Vec3 tVec = b.center - a.center;
	double t[3] = {Vec3::dot(tVec, a.axis[0]), Vec3::dot(tVec, a.axis[1]),
				   Vec3::dot(tVec, a.axis[2])};
	double ra, rb;
	double a_half[3] = {a.half.x, a.half.y, a.half.z};
	double b_half[3] = {b.half.x, b.half.y, b.half.z};

	for (int i = 0; i < 3; ++i)
	{
		ra = a_half[i];
		rb = b_half[0] * absR[i][0] + b_half[1] * absR[i][1] +
			 b_half[2] * absR[i][2];
		if (std::fabs(t[i]) > ra + rb)
			return false;
	}

	for (int i = 0; i < 3; ++i)
	{
		ra = a_half[0] * absR[0][i] + a_half[1] * absR[1][i] +
			 a_half[2] * absR[2][i];
		rb = b_half[i];
		double tval =
			std::fabs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]);
		if (tval > ra + rb)
			return false;
	}

	for (int i = 0; i < 3; ++i)
	{
		int i1 = (i + 1) % 3;
		int i2 = (i + 2) % 3;
		for (int j = 0; j < 3; ++j)
		{
			int j1 = (j + 1) % 3;
			int j2 = (j + 2) % 3;
			ra = a_half[i1] * absR[i2][j] + a_half[i2] * absR[i1][j];
			rb = b_half[j1] * absR[i][j2] + b_half[j2] * absR[i][j1];
			double tval = std::fabs(t[i2] * R[i1][j] - t[i1] * R[i2][j]);
			if (tval > ra + rb)
				return false;
		}
	}

	return true;
}

static Vec3 support(const Hittable &h, const Vec3 &dir)
{
	switch (h.shape_type())
	{
	case ShapeType::Sphere:
		return support(*static_cast<const Sphere *>(&h), dir);
	case ShapeType::Cube:
		return support(*static_cast<const Cube *>(&h), dir);
	case ShapeType::Cylinder:
		return support(*static_cast<const Cylinder *>(&h), dir);
	case ShapeType::Cone:
		return support(*static_cast<const Cone *>(&h), dir);
	default:
	{
		AABB box;
		if (h.bounding_box(box))
			return support_box(box, dir);
		return Vec3(0, 0, 0);
	}
	}
}

static bool same_direction(const Vec3 &a, const Vec3 &b)
{
	return Vec3::dot(a, b) > 0;
}

static bool handle_simplex(std::array<Vec3, 4> &simplex, int &size, Vec3 &dir)
{
	Vec3 A = simplex[size - 1];
	Vec3 AO = (-1) * A;
	if (size == 2)
	{
		Vec3 B = simplex[0];
		Vec3 AB = B - A;
		if (same_direction(AB, AO))
			dir = Vec3::cross(Vec3::cross(AB, AO), AB);
		else
		{
			simplex[0] = A;
			size = 1;
			dir = AO;
		}
		return false;
	}
	if (size == 3)
	{
		Vec3 B = simplex[0];
		Vec3 C = simplex[1];
		Vec3 AB = B - A;
		Vec3 AC = C - A;
		Vec3 ABC = Vec3::cross(AB, AC);
		if (same_direction(Vec3::cross(ABC, AC), AO))
		{
			simplex[0] = A;
			simplex[1] = C;
			size = 2;
			dir = Vec3::cross(Vec3::cross(AC, AO), AC);
			return false;
		}
		if (same_direction(Vec3::cross(AB, ABC), AO))
		{
			simplex[1] = A;
			size = 2;
			dir = Vec3::cross(Vec3::cross(AB, AO), AB);
			return false;
		}
		if (same_direction(ABC, AO))
		{
			dir = ABC;
		}
		else
		{
			std::swap(simplex[0], simplex[1]);
			dir = (-1) * ABC;
		}
		return false;
	}
	if (size == 4)
	{
		Vec3 B = simplex[0];
		Vec3 C = simplex[1];
		Vec3 D = simplex[2];
		Vec3 AB = B - A;
		Vec3 AC = C - A;
		Vec3 AD = D - A;
		Vec3 ABC = Vec3::cross(AB, AC);
		Vec3 ACD = Vec3::cross(AC, AD);
		Vec3 ADB = Vec3::cross(AD, AB);
		if (same_direction(ABC, AO))
		{
			simplex[0] = A;
			simplex[1] = B;
			simplex[2] = C;
			size = 3;
			dir = ABC;
			return false;
		}
		if (same_direction(ACD, AO))
		{
			simplex[0] = A;
			simplex[1] = C;
			simplex[2] = D;
			size = 3;
			dir = ACD;
			return false;
		}
		if (same_direction(ADB, AO))
		{
			simplex[0] = A;
			simplex[1] = D;
			simplex[2] = B;
			size = 3;
			dir = ADB;
			return false;
		}
		return true;
	}
	return false;
}

static bool gjk(const Hittable &a, const Hittable &b)
{
	Vec3 dir(1, 0, 0);
	std::array<Vec3, 4> simplex;
	int size = 0;
	simplex[size++] = support(a, dir) - support(b, (-1) * dir);
	dir = (-1) * simplex[0];
	for (int iterations = 0; iterations < 64; ++iterations)
	{
		Vec3 p = support(a, dir) - support(b, (-1) * dir);
		if (Vec3::dot(p, dir) <= 0)
			return false;
		simplex[size++] = p;
		if (handle_simplex(simplex, size, dir))
			return true;
	}
	return false;
}

} // namespace

bool precise_collision(const HittablePtr &a, const HittablePtr &b)
{
	if (!a || !b)
		return false;

	if (a->is_beam() || b->is_beam())
		return false;

	ShapeType ta = a->shape_type();
	ShapeType tb = b->shape_type();

	if (ta == ShapeType::Plane || tb == ShapeType::Plane)
	{
		const Plane *pl = (ta == ShapeType::Plane)
							  ? static_cast<const Plane *>(a.get())
							  : static_cast<const Plane *>(b.get());
		HittablePtr other = (ta == ShapeType::Plane) ? b : a;
		switch (other->shape_type())
		{
		case ShapeType::Sphere:
		{
			const Sphere *s = static_cast<const Sphere *>(other.get());
			double dist = Vec3::dot(s->center - pl->point, pl->normal);
			return std::fabs(dist) <= s->radius;
		}
		case ShapeType::Cube:
		{
			const Cube *c = static_cast<const Cube *>(other.get());
			double dist = Vec3::dot(c->center - pl->point, pl->normal);
			double r =
				std::fabs(Vec3::dot(c->axis[0], pl->normal)) * c->half.x +
				std::fabs(Vec3::dot(c->axis[1], pl->normal)) * c->half.y +
				std::fabs(Vec3::dot(c->axis[2], pl->normal)) * c->half.z;
			return std::fabs(dist) <= r;
		}
		case ShapeType::Cylinder:
		{
			const Cylinder *cy = static_cast<const Cylinder *>(other.get());
			double axial = std::fabs(Vec3::dot(cy->axis, pl->normal));
			double radial =
				std::sqrt(std::max(0.0, 1.0 - axial * axial)) * cy->radius;
			double extent = axial * (cy->height * 0.5) + radial;
			double dist =
				std::fabs(Vec3::dot(cy->center - pl->point, pl->normal));
			return dist <= extent;
		}
		case ShapeType::Cone:
		{
			const Cone *co = static_cast<const Cone *>(other.get());
			double axial = std::fabs(Vec3::dot(co->axis, pl->normal));
			double radial =
				std::sqrt(std::max(0.0, 1.0 - axial * axial)) * co->radius;
			double extent = axial * (co->height * 0.5) + radial;
			double dist =
				std::fabs(Vec3::dot(co->center - pl->point, pl->normal));
			return dist <= extent;
		}
		default:
		{
			AABB box;
			if (other->bounding_box(box))
				return box.intersects_plane(pl->point, pl->normal);
			return false;
		}
		}
	}

	if (ta == ShapeType::Cube && tb == ShapeType::Cube)
	{
		const Cube *ca = static_cast<const Cube *>(a.get());
		const Cube *cb = static_cast<const Cube *>(b.get());
		return sat_cube_cube(*ca, *cb);
	}

	if (ta == ShapeType::Sphere && tb == ShapeType::Sphere)
	{
		const Sphere *sa = static_cast<const Sphere *>(a.get());
		const Sphere *sb = static_cast<const Sphere *>(b.get());
		double rad = sa->radius + sb->radius;
		return (sa->center - sb->center).length_squared() <= rad * rad;
	}

	return gjk(*a, *b);
}
