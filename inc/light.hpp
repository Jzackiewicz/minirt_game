
#pragma once
#include "Vec3.hpp"
#include <cmath>
#include <vector>

class PointLight
{
        public:
        Vec3 position;
        Vec3 color;
        double intensity;
        std::vector<int> ignore_ids;
        int attached_id;
        Vec3 direction;
        double cutoff_cos;
        double range;
        bool reflected;
        bool beam_spotlight;
        double beam_radius;

        PointLight(const Vec3 &p, const Vec3 &c, double i,
                           std::vector<int> ignore_ids = {}, int attached_id = -1,
                           const Vec3 &dir = Vec3(0, 0, 0), double cutoff_cos = -1.0,
                           double range = -1.0, bool reflected = false,
                           bool beam_spotlight = false,
                           double beam_radius = 0.0);
};

class Ambient
{
	public:
	Vec3 color;
        double intensity;

        Ambient(const Vec3 &c, double i);
};

inline bool beam_light_geometry(const PointLight &light, const Vec3 &point,
                                                       Vec3 &axis, double &axial_dist,
                                                       Vec3 &radial_offset)
{
        if (!light.beam_spotlight)
                return false;
        double dir_len2 = light.direction.length_squared();
        if (dir_len2 <= 1e-12)
                return false;
        axis = light.direction / std::sqrt(dir_len2);
        Vec3 relative = point - light.position;
        axial_dist = Vec3::dot(relative, axis);
        if (axial_dist < 0.0)
                return false;
        if (light.range > 0.0 && axial_dist > light.range)
                return false;
        radial_offset = relative - axis * axial_dist;
        if (light.beam_radius > 0.0)
        {
                double limit = light.beam_radius * light.beam_radius + 1e-8;
                if (radial_offset.length_squared() > limit)
                        return false;
        }
        return true;
}
