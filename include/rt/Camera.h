
#pragma once
#include "Vec3.h"
#include "Ray.h"
#include <cmath>

namespace rt {

struct Camera {
    Vec3 origin;
    Vec3 forward, right, up;
    double fov_deg;
    double aspect;

    Camera(const Vec3& pos, const Vec3& look_at, double fov_deg_, double aspect_)
        : origin(pos), fov_deg(fov_deg_), aspect(aspect_)
    {
        forward = (look_at - pos).normalized();
        Vec3 world_up(0,1,0);
        right = Vec3::cross(forward, world_up).normalized();
        up = Vec3::cross(right, forward).normalized();
    }

    Ray ray_through(double u, double v) const {
        double fov_rad = fov_deg * M_PI / 180.0;
        double half_h = std::tan(fov_rad * 0.5);
        double half_w = aspect * half_h;
        Vec3 dir = (forward + (2*u-1)*half_w*right + (1-2*v)*half_h*up).normalized();
        return Ray(origin, dir);
    }
};

} // namespace rt
