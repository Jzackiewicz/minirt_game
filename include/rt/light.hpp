
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
  std::vector<int> ignore_ids;

  PointLight(const Vec3 &p, const Vec3 &c, double i,
             std::vector<int> ignore_ids = {});
};

struct Ambient
{
  Vec3 color;
  double intensity;

  Ambient(const Vec3 &c, double i);
};

} // namespace rt
