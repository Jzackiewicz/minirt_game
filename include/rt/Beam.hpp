#pragma once
#include "BeamSource.hpp"
#include "Laser.hpp"
#include "LightRay.hpp"
#include <memory>

namespace rt {
struct Beam {
  std::shared_ptr<LightRay> light;
  std::shared_ptr<Laser> laser;
  std::shared_ptr<BeamSource> source;
  Beam(const Vec3 &origin, const Vec3 &dir, double radius, double length,
       double intensity, int base_oid, int laser_mat, int big_mat,
       int mid_mat, int small_mat);
};
}
