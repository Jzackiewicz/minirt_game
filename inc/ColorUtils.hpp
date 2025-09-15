#pragma once
#include "Vec3.hpp"
#include <algorithm>

inline Vec3 mix_colors(const Vec3 &a, const Vec3 &b)
{
    Vec3 sum = a + b;
    double maxc = std::max({sum.x, sum.y, sum.z, 1.0});
    return sum / maxc;
}
