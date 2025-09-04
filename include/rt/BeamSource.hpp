#pragma once
#include "Hittable.hpp"
#include "material.hpp"
#include <vector>

namespace rt
{
std::vector<HittablePtr>
make_beam_source(const Vec3 &center, double radius, const Vec3 &color,
                 int &oid, int &mid, std::vector<Material> &materials,
                 bool movable);
}
