#pragma once
#include "Ray.hpp"

class LightRay
{
        public:
        Ray ray;
        double radius;
        double intensity;
        Vec3 color;
        double length;
        LightRay(const Vec3 &origin, const Vec3 &dir, double r, double intens,
                          const Vec3 &col, double len)
                : ray(origin, dir.normalized()), radius(r), intensity(intens),
                  color(col), length(len)
        {
        }
};
