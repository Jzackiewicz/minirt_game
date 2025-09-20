#include "BeamSource.hpp"
#include <algorithm>
#include <cmath>

namespace
{
constexpr double kSpotlightLaserRatio = 20.0;
constexpr double kDirectionEps = 1e-12;

Vec3 normalized_or_default(const Vec3 &dir)
{
        if (dir.length_squared() < kDirectionEps)
                return Vec3(0, 0, 1);
        return dir.normalized();
}
}

BeamSource::BeamSource(const Vec3 &c, const Vec3 &dir,
                                           const std::shared_ptr<Laser> &bm,
                                           const std::shared_ptr<LightRay> &lt,
                                           double mid_radius, int oid, int mat_big,
                                           int mat_mid, int mat_small)
       : Sphere(c, mid_radius * 2.0, oid, mat_big),
         mid(c, mid_radius * 1.5, -oid - 1, mat_mid),
         inner(c, mid_radius, -oid - 2, mat_small),
         orientation(c, Vec3(0, 0, -1), 1.0, 1.0, -oid - 3, mat_small, false),
         beam(bm), light(lt)
{
        update_orientation_geometry(dir);
}

bool BeamSource::hit(const Ray &r, double tmin, double tmax,
                                         HitRecord &rec) const
{
        bool hit_any = false;
        HitRecord tmp;
        double closest = tmax;
        Vec3 beam_dir = normalized_or_default(spot_direction());
        if (Sphere::hit(r, tmin, closest, tmp))
        {
                hit_any = true;
                closest = tmp.t;
                rec = tmp;
	}
	if (mid.hit(r, tmin, closest, tmp))
	{
		hit_any = true;
		closest = tmp.t;
		rec = tmp;
        }
        if (inner.hit(r, tmin, closest, tmp))
        {
                Vec3 to_hit = (tmp.p - inner.center).normalized();
                const double hole_cos = std::sqrt(1.0 - 0.25 * 0.25);
                if (Vec3::dot(beam_dir, to_hit) < hole_cos)
                {
                        hit_any = true;
                        closest = tmp.t;
                        rec = tmp;
                }
        }
        if (orientation.hit(r, tmin, closest, tmp))
        {
                hit_any = true;
                closest = tmp.t;
                rec = tmp;
        }
        return hit_any;
}

void BeamSource::translate(const Vec3 &delta)
{
        Sphere::translate(delta);
        mid.translate(delta);
        inner.translate(delta);
        orientation.translate(delta);
        if (beam)
                beam->path.orig += delta;
        if (light)
                light->ray.orig += delta;
        update_orientation_geometry(spot_direction());
}

void BeamSource::rotate(const Vec3 &ax, double angle)
{
        auto rotate_vec = [](const Vec3 &v, const Vec3 &axis, double ang)
	{
		double c = std::cos(ang);
		double s = std::sin(ang);
		return v * c + Vec3::cross(axis, v) * s +
			   axis * Vec3::dot(axis, v) * (1 - c);
	};
        if (beam)
                beam->path.dir = rotate_vec(beam->path.dir, ax, angle).normalized();
        if (light)
                light->ray.dir =
                        rotate_vec(light->ray.dir, ax, angle).normalized();
        update_orientation_geometry(spot_direction());
}

Vec3 BeamSource::spot_direction() const
{
        if (beam)
                return beam->path.dir;
        if (light)
                return light->ray.dir;
        return Vec3(0, 0, 1);
}

double BeamSource::spotlight_radius() const
{
        double base_radius = inner.radius;
        if (beam)
                base_radius = beam->radius;
        else if (light)
                base_radius = light->radius;
        else if (radius > 0.0)
                base_radius = radius * 0.5;
        base_radius = std::max(base_radius, 0.0);
        return base_radius * kSpotlightLaserRatio;
}

void BeamSource::update_orientation_geometry(const Vec3 &dir)
{
        Vec3 direction = dir;
        if (direction.length_squared() < kDirectionEps)
        {
                Vec3 fallback = spot_direction();
                if (fallback.length_squared() < kDirectionEps)
                        fallback = Vec3(0, 0, 1);
                direction = fallback;
        }
        direction = normalized_or_default(direction);

        double height = radius;
        if (height <= 0.0)
                height = 1e-4;

        double base_radius = spotlight_radius();

        orientation.height = height;
        orientation.radius = base_radius;
        Vec3 axis = (-direction);
        if (axis.length_squared() < kDirectionEps)
                axis = Vec3(0, 0, -1);
        orientation.axis = axis.normalized();
        orientation.center = center + direction * (height * 0.5);
}

bool BeamSource::bounding_box(AABB &out) const
{
        if (!Sphere::bounding_box(out))
                return false;
        AABB tmp;
        if (mid.bounding_box(tmp))
                out = AABB::surrounding_box(out, tmp);
        if (inner.bounding_box(tmp))
                out = AABB::surrounding_box(out, tmp);
        if (orientation.bounding_box(tmp))
                out = AABB::surrounding_box(out, tmp);
        return true;
}
