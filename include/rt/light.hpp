
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
  double girth;
  double length;
  double start;
  double total_length;
  bool is_beam;

  PointLight(const Vec3 &p, const Vec3 &c, double i,
             std::vector<int> ignore_ids = {}, int attached_id = -1,
             const Vec3 &dir = Vec3(0, 0, 0), double cutoff_cos = -1.0,
             double girth = -1.0, double length = -1.0,
             double total_length = -1.0, double start = 0.0,
             bool is_beam = false);
};

struct Ambient
{
  Vec3 color;
  double intensity;

  Ambient(const Vec3 &c, double i);
};

} // namespace rt
