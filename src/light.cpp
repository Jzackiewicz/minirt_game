#include "rt/light.hpp"

namespace rt
{
PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i)
    : position(p), color(c), intensity(i)
{
}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}

} // namespace rt
