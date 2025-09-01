
#pragma once
#include "Vec3.hpp"
#include <vector>

namespace rt
{
struct PointLight
{
  Vec3 position;
  Vec3 color;
  double intensity;
  int ignore_id;

  PointLight(const Vec3 &p, const Vec3 &c, double i, int ignore = -1);
};

struct Ambient
{
  Vec3 color;
  double intensity;

  Ambient(const Vec3 &c, double i);
};

} // namespace rt
