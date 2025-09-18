#include "light.hpp"
#include <utility>

PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i,
                                           std::vector<int> ignore, int attached_id,
                                           const Vec3 &dir, double cutoff, double range,
                                           bool refl, bool beam, double radius)
        : position(p), color(c), intensity(i), ignore_ids(std::move(ignore)),
          attached_id(attached_id), direction(dir), cutoff_cos(cutoff), range(range),
          reflected(refl), beam_light(beam), beam_radius(radius)
{
}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}
