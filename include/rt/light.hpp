
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
  Vec3 direction;
  double range;
  double cos_girth;
  std::vector<int> ignore_ids;
  int attached_id;

  PointLight(const Vec3 &p, const Vec3 &c, double i, const Vec3 &dir = Vec3(0, 0, 0),
             double range = 0.0, double cos_girth = -1.0,
             std::vector<int> ignore_ids = {}, int attached_id = -1);
};

struct Ambient
{
  Vec3 color;
  double intensity;

  Ambient(const Vec3 &c, double i);
};

} // namespace rt
