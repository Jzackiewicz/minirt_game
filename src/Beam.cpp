#include "rt/Beam.hpp"

namespace rt {
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
           double intensity, int &next_oid, int light_mat, int mat_big,
           int mat_mid, int mat_small)
{
  light = std::make_shared<LightRay>(origin, dir, radius, length, intensity,
                                     next_oid++, light_mat);
  source = std::make_shared<BeamSource>(origin, dir, light, radius, next_oid++,
                                        mat_big, mat_mid, mat_small);
  light->source = source;
  laser = std::make_shared<Laser>(light);
}
} // namespace rt
