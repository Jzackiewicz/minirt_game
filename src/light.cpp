#include "light.hpp"
#include <utility>

PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i,
                                           std::vector<int> ignore_ids, int attached_id,
                                           const Vec3 &dir, double cutoff, double range,
                                           bool reflected, double rad)
        : position(p), color(c), intensity(i), ignore_ids(std::move(ignore_ids)),
          attached_id(attached_id), direction(dir), cutoff_cos(cutoff), range(range),
          reflected(reflected), radius(rad)
{
}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}
