#include "rt/light.hpp"
#include <utility>

namespace rt
{
PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i, const Vec3 &dir,
                       double range, double cos_girth,
                       std::vector<int> ignore_ids, int attached_id)
    : position(p), color(c), intensity(i), direction(dir.normalized()),
      range(range), cos_girth(cos_girth), ignore_ids(std::move(ignore_ids)),
      attached_id(attached_id)
{}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}

} // namespace rt
