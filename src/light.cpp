#include "rt/light.hpp"

namespace rt
{
PointLight::PointLight(const Vec3 &p, const Vec3 &c, double i,
                       const Hittable *ig1, const Hittable *ig2)
    : position(p), color(c), intensity(i), ignore1(ig1), ignore2(ig2)
{
}

Ambient::Ambient(const Vec3 &c, double i) : color(c), intensity(i) {}

} // namespace rt
