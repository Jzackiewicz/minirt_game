
#pragma once
#include "Vec3.hpp"
#include "light.hpp"
#include <vector>

#define REFLECTION_FACTOR 20

namespace rt
{
struct Material
{
  Vec3 color;
  double alpha = 1.0;
  double specular_exp = 50.0;
  double specular_k = 0.5;
  bool mirror = false;
  bool random_alpha = false;
};

Vec3 phong(const Material &m, const Ambient &ambient,
           const std::vector<PointLight> &lights, const Vec3 &p, const Vec3 &n,
           const Vec3 &eye);

} // namespace rt
