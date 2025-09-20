#include "light.hpp"
#include <utility>
#include <cmath>

PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i,
                                          std::vector<int> ignore_ids, int attached_id,
                                          const Vec3 &dir, double cutoff, double range,
                                          bool reflected, bool beam_light, double spot_r)
        : position(p), color(c), intensity(i), ignore_ids(std::move(ignore_ids)),
          attached_id(attached_id), direction(dir), cutoff_cos(cutoff), range(range),
          reflected(reflected), beam_spotlight(beam_light), spot_radius(spot_r)
{
}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}

bool spotlight_covers_point(const PointLight &light, const Vec3 &point,
                                                   double *axial_distance)
{
        if (light.beam_spotlight && light.spot_radius > 0.0)
        {
                Vec3 axis = light.direction;
                double axis_len = axis.length();
                if (axis_len <= 1e-9)
                {
                        if (axial_distance)
                                *axial_distance = 0.0;
                        return false;
                }
                Vec3 dir_norm = axis / axis_len;
                Vec3 rel = point - light.position;
                double axial = Vec3::dot(rel, dir_norm);
                if (axial < 0.0)
                {
                        if (axial_distance)
                                *axial_distance = axial;
                        return false;
                }
                if (light.range > 0.0 && axial > light.range)
                {
                        if (axial_distance)
                                *axial_distance = axial;
                        return false;
                }
                Vec3 radial = rel - dir_norm * axial;
                if (radial.length_squared() > light.spot_radius * light.spot_radius)
                {
                        if (axial_distance)
                                *axial_distance = axial;
                        return false;
                }
                if (axial_distance)
                        *axial_distance = axial;
                return true;
        }
        if (light.cutoff_cos > -1.0)
        {
                Vec3 dir = light.direction;
                double len2 = dir.length_squared();
                if (len2 <= 1e-12)
                {
                        if (axial_distance)
                                *axial_distance = 0.0;
                        return false;
                }
                Vec3 dir_norm = dir / std::sqrt(len2);
                Vec3 rel = point - light.position;
                double rel_len2 = rel.length_squared();
                if (rel_len2 <= 1e-12)
                {
                        if (axial_distance)
                                *axial_distance = 0.0;
                        return true;
                }
                Vec3 spot_dir = rel / std::sqrt(rel_len2);
                if (Vec3::dot(dir_norm, spot_dir) < light.cutoff_cos)
                        return false;
        }
        if (axial_distance)
                *axial_distance = (point - light.position).length();
        return true;
}
