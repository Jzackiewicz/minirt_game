#include "rt/light.hpp"
#include <utility>

namespace rt
{
PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i,
                       std::vector<int> ignore_ids)
    : position(p), color(c), intensity(i),
      ignore_ids(std::move(ignore_ids))
{}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}

} // namespace rt
