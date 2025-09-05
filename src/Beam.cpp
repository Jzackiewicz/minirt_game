#include "rt/Beam.hpp"

namespace rt {
Beam::Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
           double intensity, int base_oid, int laser_mat, int big_mat,
           int mid_mat, int small_mat) {
  light = std::make_shared<LightRay>(origin, dir, intensity);
  laser = std::make_shared<Laser>(origin, dir, radius, length, intensity,
                                  base_oid, laser_mat);
  source = std::make_shared<BeamSource>(origin, dir, laser, radius,
                                        base_oid + 1, big_mat, mid_mat,
                                        small_mat);
  laser->source = source;
}
} // namespace rt
