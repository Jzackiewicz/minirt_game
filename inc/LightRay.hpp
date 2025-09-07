#pragma once
#include "Ray.hpp"

class LightRay
{
        public:
        Ray ray;
       double radius;
       double intensity;
       LightRay(const Vec3 &origin, const Vec3 &dir, double r, double intens)
               : ray(origin, dir.normalized()), radius(r), intensity(intens)
       {
       }
};
