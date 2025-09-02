#include "rt/material.hpp"
#include <algorithm>
#include <cmath>

namespace rt
{
Vec3 phong(const Material &m, const Ambient &ambient,
           const std::vector<PointLight> &lights, const Vec3 &p, const Vec3 &n,
           const Vec3 &eye)
{
  Vec3 base = m.base_color;
  Vec3 col = m.color;
  if (m.checkered)
  {
    Vec3 inv = Vec3(1.0, 1.0, 1.0) - base;
    int chk = (static_cast<int>(std::floor(p.x * 5)) +
               static_cast<int>(std::floor(p.y * 5)) +
               static_cast<int>(std::floor(p.z * 5))) & 1;
    col = chk ? base : inv;
  }
  Vec3 c(0, 0, 0);
  c = Vec3(col.x * ambient.color.x * ambient.intensity,
           col.y * ambient.color.y * ambient.intensity,
           col.z * ambient.color.z * ambient.intensity);
  for (const auto &L : lights)
  {
    Vec3 from_light = p - L.position;
    double dist_to_light = from_light.length();
    if (L.range > 0.0 && dist_to_light > L.range)
      continue;
    Vec3 ldir = (from_light * -1.0).normalized();
    if (L.girth > 0.0 && L.direction.length_squared() > 0.0)
    {
      double along = Vec3::dot(from_light, L.direction);
      if (along < 0.0 || (L.range > 0.0 && along > L.range))
        continue;
      Vec3 radial = from_light - L.direction * along;
      if (radial.length_squared() > L.girth * L.girth)
        continue;
    }
    else if (L.cutoff_cos > -1.0)
    {
      Vec3 spot_dir = from_light.normalized();
      if (Vec3::dot(L.direction, spot_dir) < L.cutoff_cos)
        continue;
    }
    double diff = std::max(0.0, Vec3::dot(n, ldir));
    Vec3 h = (ldir + eye).normalized();
    double spec =
        std::pow(std::max(0.0, Vec3::dot(n, h)), m.specular_exp) * m.specular_k;
    double intensity = L.intensity;
    if (L.range > 0.0)
      intensity *= std::max(0.0, 1.0 - dist_to_light / L.range);
    c += Vec3(col.x * L.color.x * intensity * diff + L.color.x * spec,
              col.y * L.color.y * intensity * diff + L.color.y * spec,
              col.z * L.color.z * intensity * diff + L.color.z * spec);
  }
  return c;
}

} // namespace rt
