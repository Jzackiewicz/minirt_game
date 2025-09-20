#include "BeamSource.hpp"
#include "AABB.hpp"
#include <algorithm>
#include <cmath>

BeamSource::BeamSource(const Vec3 &c, const Vec3 &dir,
                                           const std::shared_ptr<Laser> &bm,
                                           const std::shared_ptr<LightRay> &lt,
                                           double mid_radius, int oid, int mat_big,
                                           int mat_mid, int mat_small)
       : Sphere(c, mid_radius * 2.0, oid, mat_big),
         mid(c, mid_radius * 1.5, -oid - 1, mat_mid),
         inner(c, mid_radius, -oid - 2, mat_small), beam(bm), light(lt)
{
        Vec3 dir_norm = dir.normalized();
        cone_axis = (-1.0) * dir_norm;
        constexpr double kSpotlightLaserRatio = 20.0;
        cone_radius = 0.0;
        if (beam)
        {
                cone_radius = beam->radius * kSpotlightLaserRatio;
        }
        else if (light)
        {
                cone_radius = light->radius * kSpotlightLaserRatio;
        }
        else
        {
                cone_radius = mid_radius * kSpotlightLaserRatio;
        }
        cone_height = cone_radius;
        if (cone_height <= 0.0)
        {
                cone_radius = 0.0;
                cone_height = 0.0;
        }
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
                Vec3 beam_dir = beam
                                        ? beam->path.dir
                                        : (light ? light->ray.dir : Vec3(0, 0, 1));
                Vec3 to_hit = (tmp.p - inner.center).normalized();
                const double hole_cos = std::sqrt(1.0 - 0.25 * 0.25);
                if (Vec3::dot(beam_dir, to_hit) < hole_cos)
                {
                        hit_any = true;
                        closest = tmp.t;
                        rec = tmp;
                }
        }
        if (hit_cone(r, tmin, closest, tmp))
        {
                hit_any = true;
                closest = tmp.t;
                rec = tmp;
        }
        return hit_any;
}

bool BeamSource::bounding_box(AABB &out) const
{
        AABB sphere_box;
        Sphere::bounding_box(sphere_box);
        if (cone_radius <= 0.0 || cone_height <= 0.0)
        {
                out = sphere_box;
                return true;
        }

        Vec3 apex = inner.center;
        Vec3 axis = cone_axis;
        Vec3 base_center = apex - axis * cone_height;

        Vec3 arbitrary = std::fabs(axis.x) > 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        Vec3 u = Vec3::cross(arbitrary, axis).normalized();
        Vec3 v = Vec3::cross(axis, u).normalized();

        double proj_x = cone_radius * std::sqrt(u.x * u.x + v.x * v.x);
        double proj_y = cone_radius * std::sqrt(u.y * u.y + v.y * v.y);
        double proj_z = cone_radius * std::sqrt(u.z * u.z + v.z * v.z);

        Vec3 cone_min(std::min(apex.x, base_center.x - proj_x),
                       std::min(apex.y, base_center.y - proj_y),
                       std::min(apex.z, base_center.z - proj_z));
        Vec3 cone_max(std::max(apex.x, base_center.x + proj_x),
                       std::max(apex.y, base_center.y + proj_y),
                       std::max(apex.z, base_center.z + proj_z));

        AABB cone_box(cone_min, cone_max);
        out = AABB::surrounding_box(sphere_box, cone_box);
        return true;
}

void BeamSource::translate(const Vec3 &delta)
{
        Sphere::translate(delta);
        mid.translate(delta);
        inner.translate(delta);
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
        cone_axis = rotate_vec(cone_axis, ax, angle).normalized();
}

Vec3 BeamSource::spot_direction() const
{
        if (beam)
                return beam->path.dir;
        if (light)
                return light->ray.dir;
        return Vec3(0, 0, 1);
}

bool BeamSource::hit_cone(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
        if (cone_radius <= 0.0 || cone_height <= 0.0)
        {
                return false;
        }

        Vec3 apex = inner.center;
        Vec3 axis = cone_axis;
        Vec3 down = (-1.0) * axis;
        double k = cone_radius / cone_height;

        Vec3 oc = r.orig - apex;
        double oc_dot_d = Vec3::dot(oc, down);
        double d_dot_d = Vec3::dot(r.dir, down);

        Vec3 oc_perp = oc - oc_dot_d * down;
        Vec3 d_perp = r.dir - d_dot_d * down;

        double A = Vec3::dot(d_perp, d_perp) - k * k * d_dot_d * d_dot_d;
        double B = 2.0 * Vec3::dot(d_perp, oc_perp) - 2.0 * k * k * d_dot_d * oc_dot_d;
        double C = Vec3::dot(oc_perp, oc_perp) - k * k * oc_dot_d * oc_dot_d;

        double disc = B * B - 4.0 * A * C;
        if (disc < 0.0)
        {
                return false;
        }

        double denom = 2.0 * A;
        if (std::fabs(denom) < 1e-12)
        {
                return false;
        }

        double sqrt_disc = std::sqrt(disc);
        double roots[2] = {(-B - sqrt_disc) / denom, (-B + sqrt_disc) / denom};
        bool hit_any = false;
        double closest = tmax;
        HitRecord best_rec;

        for (double root : roots)
        {
                if (root < tmin || root > tmax)
                {
                        continue;
                }
                double y = oc_dot_d + root * d_dot_d;
                if (y < 0.0 || y > cone_height)
                {
                        continue;
                }
                Vec3 p = r.at(root);
                double ax_dist = y;
                Vec3 x_parallel = down * ax_dist;
                Vec3 x_perp = (oc + root * r.dir) - x_parallel;
                Vec3 normal = (x_perp - (k * k * ax_dist) * down).normalized();

                HitRecord candidate;
                candidate.t = root;
                candidate.p = p;
                candidate.object_id = object_id;
                candidate.material_id = inner.material_id;
                candidate.set_face_normal(r, normal);

                if (candidate.t < closest)
                {
                        closest = candidate.t;
                        best_rec = candidate;
                        hit_any = true;
                }
        }

        if (!hit_any)
        {
                return false;
        }

        rec = best_rec;
        return true;
}
