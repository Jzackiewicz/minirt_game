#include "BeamSource.hpp"
#include <cmath>

namespace
{
constexpr double kSpotlightLaserRatio = 20.0;

Vec3 normalized_or_default(const Vec3 &dir)
{
        if (dir.length_squared() == 0.0)
                return Vec3(0, 0, 1);
        return dir.normalized();
}

double compute_cone_radius(double mid_radius,
                                                  const std::shared_ptr<Laser> &beam,
                                                  const std::shared_ptr<LightRay> &light)
{
        if (beam)
                return beam->radius * kSpotlightLaserRatio;
        if (light)
                return light->radius * kSpotlightLaserRatio;
        return mid_radius * kSpotlightLaserRatio;
}

Cone make_beam_cone(const Vec3 &origin, const Vec3 &dir, double outer_radius,
                                        double mid_radius, int object_id, int material_id,
                                        const std::shared_ptr<Laser> &beam,
                                        const std::shared_ptr<LightRay> &light)
{
        Vec3 direction = normalized_or_default(dir);
        double radius = compute_cone_radius(mid_radius, beam, light);
        double height = outer_radius;
        Vec3 axis = (-direction).normalized();
        Vec3 cone_center = origin - axis * (height * 0.5);
        return Cone(cone_center, axis, radius, height, object_id, material_id, false);
}

void align_cone(Cone &cone, const Vec3 &origin, const Vec3 &direction)
{
        Vec3 dir_norm = normalized_or_default(direction);
        cone.axis = (-dir_norm).normalized();
        double half_height = cone.height * 0.5;
        cone.center = origin - cone.axis * half_height;
}
} // namespace

BeamSource::BeamSource(const Vec3 &c, const Vec3 &dir,
                                           const std::shared_ptr<Laser> &bm,
                                           const std::shared_ptr<LightRay> &lt,
                                           double mid_radius, int oid, int mat_big,
                                           int mat_mid, int mat_small)
       : Sphere(c, mid_radius * 2.0, oid, mat_big),
         mid(c, mid_radius * 1.5, -oid - 1, mat_mid),
         inner(c, mid_radius, -oid - 2, mat_small),
         cone(make_beam_cone(c, dir, mid_radius * 2.0, mid_radius, -oid - 3,
                                                 mat_mid, bm, lt)),
         beam(bm), light(lt)
{
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
                Vec3 beam_dir = normalized_or_default(spot_direction());
                Vec3 to_hit = (tmp.p - inner.center).normalized();
                const double hole_cos = std::sqrt(1.0 - 0.25 * 0.25);
                if (Vec3::dot(beam_dir, to_hit) < hole_cos)
                {
                        hit_any = true;
                        closest = tmp.t;
			rec = tmp;
		}
        }
        if (cone.hit(r, tmin, closest, tmp))
        {
                hit_any = true;
                closest = tmp.t;
                rec = tmp;
        }
        return hit_any;
}

bool BeamSource::bounding_box(AABB &out) const
{
        AABB outer_box;
        if (!Sphere::bounding_box(outer_box))
                return false;
        AABB mid_box;
        mid.bounding_box(mid_box);
        outer_box = AABB::surrounding_box(outer_box, mid_box);
        AABB inner_box;
        inner.bounding_box(inner_box);
        outer_box = AABB::surrounding_box(outer_box, inner_box);
        AABB cone_box;
        cone.bounding_box(cone_box);
        out = AABB::surrounding_box(outer_box, cone_box);
        return true;
}

void BeamSource::translate(const Vec3 &delta)
{
        Sphere::translate(delta);
        mid.translate(delta);
        inner.translate(delta);
        cone.translate(delta);
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
        Vec3 current_dir = spot_direction();
        Vec3 rotated = rotate_vec(normalized_or_default(current_dir), ax, angle);
        Vec3 new_dir = normalized_or_default(rotated);
        if (beam)
                beam->path.dir = new_dir;
        if (light)
                light->ray.dir = new_dir;
        align_cone(cone, center, new_dir);
}

Vec3 BeamSource::spot_direction() const
{
        if (beam)
                return beam->path.dir;
        if (light)
                return light->ray.dir;
        if (cone.axis.length_squared() == 0.0)
                return Vec3(0, 0, 1);
        return (-cone.axis).normalized();
}
