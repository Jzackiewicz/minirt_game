#include "rt/light.hpp"
#include <utility>

namespace rt
{
PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i, double range,
                       std::vector<int> ignore_ids, int attached_id,
                       const Vec3 &dir, double cutoff)
    : position(p), color(c), intensity(i), range(range),
      ignore_ids(std::move(ignore_ids)), attached_id(attached_id),
      direction(dir), cutoff_cos(cutoff)
{}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}

} // namespace rt
