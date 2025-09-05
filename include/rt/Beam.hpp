#pragma once
#include <memory>
#include "LightRay.hpp"
#include "BeamSource.hpp"
#include "Laser.hpp"

namespace rt {
struct Beam {
  std::shared_ptr<BeamSource> source;
  std::shared_ptr<Laser> laser;
  std::shared_ptr<LightRay> light;
  Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
       double intensity, int &next_oid, int light_mat, int mat_big,
       int mat_mid, int mat_small);
};
} // namespace rt
