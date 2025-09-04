#pragma once
#include "Ray.hpp"
#include <memory>

namespace rt
{
struct Hittable; // forward declaration

struct Beam
{
  Ray path;
  double length;
  double start;
  double total_length;
  double light_intensity;
  int material_id;
  std::weak_ptr<Hittable> source;

  Beam(const Vec3 &origin, const Vec3 &dir, double length, double intensity,
       int mid, double start = 0.0, double total = -1.0)
      : path(origin, dir.normalized()), length(length), start(start),
        total_length(total < 0 ? length : total), light_intensity(intensity),
        material_id(mid)
  {
  }
};

} // namespace rt
