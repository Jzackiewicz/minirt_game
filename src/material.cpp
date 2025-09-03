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
    Vec3 ldir;
    double intensity = L.intensity;
    if (L.is_beam)
    {
      ldir = (L.direction * -1.0).normalized();
      Vec3 w = p - L.position;
      double t = Vec3::dot(w, L.direction);
      intensity *= std::max(0.0, 1.0 - (L.start + t) / L.total_length);
    }
    else
    {
      ldir = (L.position - p).normalized();
    }
    if (L.cutoff_cos > -1.0)
    {
      Vec3 spot_dir = (p - L.position).normalized();
      if (Vec3::dot(L.direction, spot_dir) < L.cutoff_cos)
        continue;
    }
    double diff = std::max(0.0, Vec3::dot(n, ldir));
    Vec3 h = (ldir + eye).normalized();
    double spec =
        std::pow(std::max(0.0, Vec3::dot(n, h)), m.specular_exp) * m.specular_k;
    c += Vec3(col.x * L.color.x * intensity * diff + L.color.x * spec,
              col.y * L.color.y * intensity * diff + L.color.y * spec,
              col.z * L.color.z * intensity * diff + L.color.z * spec);
  }
  return c;
}

} // namespace rt
