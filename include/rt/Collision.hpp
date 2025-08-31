#pragma once
#include "Hittable.hpp"
#include <vector>

namespace rt
{

bool precise_collision(const HittablePtr &a, const HittablePtr &b);

}
