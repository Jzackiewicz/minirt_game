
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
  int attached_id;
  Vec3 direction;
  double cutoff_cos;
  double beam_radius;
  double beam_length;

  PointLight(const Vec3 &p, const Vec3 &c, double i,
             std::vector<int> ignore_ids = {}, int attached_id = -1,
             const Vec3 &dir = Vec3(0, 0, 0), double cutoff_cos = -1.0,
             double beam_radius = -1.0, double beam_length = -1.0);
};

struct Ambient
{
  Vec3 color;
  double intensity;

  Ambient(const Vec3 &c, double i);
};

} // namespace rt
