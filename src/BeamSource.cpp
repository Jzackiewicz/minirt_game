#include "BeamSource.hpp"
#include <cmath>

namespace
{
constexpr double kCapNormalThreshold = 1.0 - 1e-6;
}

BeamSource::BeamSource(const Vec3 &c, const Vec3 &dir,
                                           const std::shared_ptr<Laser> &bm,
                                           const std::shared_ptr<LightRay> &lt,
                                           double mid_radius, int oid, int mat_big,
                                           int mat_mid, int mat_small)
       : Sphere(c, mid_radius * 2.0, oid, mat_big),
         mid(c, mid_radius * 1.5, -oid - 1, mat_mid),
         inner(c, mid_radius, -oid - 2, mat_small),
         indicator([&]() {
                 Vec3 direction = dir.length_squared() > 0.0 ? dir.normalized()
                                                            : Vec3(0, 0, 1);
                 double base_unit = mid_radius;
                 if (bm)
                         base_unit = bm->radius;
                 else if (lt)
                         base_unit = lt->radius;
                 double height = mid_radius * 2.0;
                 double radius = base_unit * kSpotlightLaserRatio;
                 Vec3 axis = (-direction).normalized();
                 Vec3 center_pos = c - axis * (height * 0.5);
                 return Cone(center_pos, axis, radius, height, -oid - 3, mat_small);
         }()),
         beam(bm),
         light(lt),
         fallback_direction(dir.length_squared() > 0.0 ? dir.normalized()
                                                       : Vec3(0, 0, 1)),
         indicator_height(mid_radius * 2.0),
         indicator_radius(((bm ? bm->radius
                               : (lt ? lt->radius : mid_radius)) *
                          kSpotlightLaserRatio))
{
        if (fallback_direction.length_squared() == 0.0)
                fallback_direction = Vec3(0, 0, 1);
        if (!(indicator_radius > 0.0))
                indicator_radius = indicator_height;
        update_indicator_geometry(spot_direction());
}

bool BeamSource::hit(const Ray &r, double tmin, double tmax,
                                         HitRecord &rec) const
{
        bool hit_any = false;
	HitRecord tmp;
	double closest = tmax;
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
                Vec3 beam_dir = spot_direction();
                Vec3 to_hit = (tmp.p - inner.center).normalized();
                const double hole_cos = std::sqrt(1.0 - 0.25 * 0.25);
                if (Vec3::dot(beam_dir, to_hit) < hole_cos)
                {
                        hit_any = true;
                        closest = tmp.t;
                        rec = tmp;
                }
        }
        if (indicator.hit(r, tmin, closest, tmp))
        {
                double dot = Vec3::dot(tmp.normal, indicator.axis);
                if (dot > -kCapNormalThreshold)
                {
                        hit_any = true;
                        closest = tmp.t;
                        rec = tmp;
                }
        }
        return hit_any;
}

bool BeamSource::bounding_box(AABB &out) const
{
        AABB outer_box;
        if (!Sphere::bounding_box(outer_box))
                return false;
        AABB cone_box;
        if (!indicator.bounding_box(cone_box))
        {
                out = outer_box;
                return true;
        }
        out = AABB::surrounding_box(outer_box, cone_box);
        return true;
}

void BeamSource::translate(const Vec3 &delta)
{
        Sphere::translate(delta);
        mid.translate(delta);
        inner.translate(delta);
        indicator.translate(delta);
        if (beam)
                beam->path.orig += delta;
        if (light)
                light->ray.orig += delta;
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
        fallback_direction = rotate_vec(fallback_direction, ax, angle).normalized();
        update_indicator_geometry(spot_direction());
}

Vec3 BeamSource::spot_direction() const
{
        if (beam)
                return beam->path.dir;
        if (light)
                return light->ray.dir;
        return fallback_direction;
}

void BeamSource::update_indicator_geometry(const Vec3 &direction)
{
        Vec3 dir = direction;
        if (dir.length_squared() == 0.0)
                dir = Vec3(0, 0, 1);
        dir = dir.normalized();
        indicator.axis = (-dir).normalized();
        indicator.height = indicator_height;
        indicator.radius = indicator_radius;
        indicator.center = center - indicator.axis * (indicator.height * 0.5);
}
