
#pragma once
#include "Vec3.hpp"

namespace rt
{
struct Ray
{
  Vec3 orig;
  Vec3 dir;

  Ray();
  Ray(const Vec3 &o, const Vec3 &d);

  Vec3 at(double t) const;
};
} // namespace rt
