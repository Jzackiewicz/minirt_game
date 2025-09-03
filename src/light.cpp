#include "rt/light.hpp"
#include <utility>

namespace rt
{
PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i,
                       std::vector<int> ignore_ids, int attached_id,
                       const Vec3 &dir, double cutoff, double len)
    : position(p), color(c), intensity(i),
      ignore_ids(std::move(ignore_ids)), attached_id(attached_id),
      direction(dir), cutoff_cos(cutoff), length(len)
{}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}

} // namespace rt
