
#pragma once
#include "Vec3.hpp"

namespace rt
{
struct Hittable;

struct PointLight
{
  Vec3 position;
  Vec3 color;
  double intensity;
  const Hittable *ignore1;
  const Hittable *ignore2;

  PointLight(const Vec3 &p, const Vec3 &c, double i,
             const Hittable *ig1 = nullptr,
             const Hittable *ig2 = nullptr);
};

struct Ambient
{
  Vec3 color;
  double intensity;

  Ambient(const Vec3 &c, double i);
};

} // namespace rt
