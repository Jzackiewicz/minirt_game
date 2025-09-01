
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
  double range;
  std::vector<int> ignore_ids;
  int attached_id;
  Vec3 direction;
  double cutoff_cos;

  PointLight(const Vec3 &p, const Vec3 &c, double i,
             std::vector<int> ignore_ids = {}, int attached_id = -1,
             const Vec3 &dir = Vec3(0, 0, 0), double cutoff_cos = -1.0,
             double range = 1e9);
};

struct Ambient
{
  Vec3 color;
  double intensity;

  Ambient(const Vec3 &c, double i);
};

} // namespace rt
