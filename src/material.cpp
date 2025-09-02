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
    Vec3 to_pt = p - L.position;
    double falloff = 1.0;
    if (L.girth > 0 || L.range > 0 || L.direction.length_squared() > 0.0)
    {
      double t = Vec3::dot(to_pt, L.direction);
      if (t < 0.0)
        continue;
      if (L.range > 0.0 && t > L.range)
        continue;
      Vec3 radial = to_pt - L.direction * t;
      if (L.girth > 0.0 && radial.length_squared() > L.girth * L.girth)
        continue;
      if (L.range > 0.0)
        falloff = std::max(0.0, 1.0 - t / L.range);
    }
    Vec3 ldir = (L.position - p).normalized();
    double diff = std::max(0.0, Vec3::dot(n, ldir));
    Vec3 h = (ldir + eye).normalized();
    double spec =
        std::pow(std::max(0.0, Vec3::dot(n, h)), m.specular_exp) * m.specular_k;
    Vec3 contrib(col.x * L.color.x * L.intensity * diff + L.color.x * spec,
                 col.y * L.color.y * L.intensity * diff + L.color.y * spec,
                 col.z * L.color.z * L.intensity * diff + L.color.z * spec);
    c += contrib * falloff;
  }
  return c;
}

} // namespace rt
