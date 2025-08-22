
#pragma once
#include "Vec3.h"
#include "light.h"

namespace rt {

struct Material {
    Vec3 color; // 0..1
    double specular_exp = 50.0;
    double specular_k = 0.5;
    bool mirror = false;
};

inline Vec3 phong(const Material& m, const Ambient& ambient,
                  const std::vector<PointLight>& lights,
                  const Vec3& p, const Vec3& n, const Vec3& eye) {
    Vec3 c(0,0,0);
    // That line is wrong; fix to per channel:
    c = Vec3(m.color.x * ambient.color.x * ambient.intensity,
             m.color.y * ambient.color.y * ambient.intensity,
             m.color.z * ambient.color.z * ambient.intensity);
    for (const auto& L : lights) {
        Vec3 ldir = (L.position - p).normalized();
        double diff = std::max(0.0, Vec3::dot(n, ldir));
        Vec3 h = (ldir + eye).normalized();
        double spec = std::pow(std::max(0.0, Vec3::dot(n, h)), m.specular_exp) * m.specular_k;
        c += Vec3(m.color.x * L.color.x * L.intensity * diff + L.color.x * spec,
                  m.color.y * L.color.y * L.intensity * diff + L.color.y * spec,
                  m.color.z * L.color.z * L.intensity * diff + L.color.z * spec);
    }
    return c;
}

} // namespace rt
