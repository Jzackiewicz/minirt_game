#pragma once
#include "Hittable.hpp"
#include "light.hpp"
#include <vector>

namespace rt
{
struct Beam
{
  std::vector<HittablePtr> parts;
  PointLight light;
  bool movable = false;
  Beam() = default;
  Beam(std::vector<HittablePtr> p, const PointLight &l, bool m)
      : parts(std::move(p)), light(l), movable(m) {}
};

PointLight make_beam_light(const Vec3 &center, const Vec3 &dir,
                           const Vec3 &color, double intensity,
                           double radius, double length,
                           const std::vector<int> &ignore_ids);

HittablePtr make_laser(const Vec3 &center, const Vec3 &dir, double radius,
                       double length, int &oid, int mid, bool movable);

} // namespace rt
