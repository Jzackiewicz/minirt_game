#include "rt/light.hpp"

namespace rt
{
PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i, int ignore)
    : position(p), color(c), intensity(i), ignore_id(ignore)
{
}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}

} // namespace rt
