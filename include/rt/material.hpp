
#pragma once
#include "Vec3.hpp"
#include "light.hpp"
#include <vector>

#define REFLECTION 50

namespace rt
{
struct Material
{
  Vec3 color;      // current color used for rendering
  Vec3 base_color; // original color stored for edits
  double alpha = 1.0;
  double specular_exp = 50.0;
  double specular_k = 0.5;
  bool mirror = false;
  bool beam_falloff = false;  // fade alpha along beam length
  bool checkered = false;     // render as checkered pattern when true
  bool unlit = false;         // render without lighting when true
  bool casts_shadow = true;   // participate in shadow tests
  bool solid = true;          // blocks movement and collisions
};

Vec3 phong(const Material &m, const Ambient &ambient,
           const std::vector<PointLight> &lights, const Vec3 &p, const Vec3 &n,
           const Vec3 &eye);

} // namespace rt
