#include "rt/light.hpp"
#include <utility>

namespace rt
{
PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i,
                       std::vector<int> ignore_ids, int attached_id,
                       const Vec3 &dir, double cutoff, double girth,
                       double length, double total_length, double start,
                       bool is_beam)
    : position(p), color(c), intensity(i),
      ignore_ids(std::move(ignore_ids)), attached_id(attached_id),
      direction(dir), cutoff_cos(cutoff), girth(girth), length(length),
      start(start), total_length(total_length), is_beam(is_beam)
{}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}

} // namespace rt
