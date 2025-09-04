#include "rt/Beam.hpp"
#include "rt/BeamSource.hpp"
#include "rt/Cylinder.hpp"
#include "rt/Sphere.hpp"
#include <cmath>

namespace rt
{
PointLight make_beam_light(const Vec3 &center, const Vec3 &dir,
                           const Vec3 &color, double intensity,
                           double radius, double length,
                           const std::vector<int> &ignore_ids)
{
  double cone_angle = std::atan((radius * 3.0) / length);
  double cutoff = std::cos(cone_angle);
  return PointLight(center, color, intensity, ignore_ids, -1,
                    dir.normalized(), cutoff, length);
}

HittablePtr make_laser(const Vec3 &center, const Vec3 &dir, double radius,
                       double length, int &oid, int mid, bool movable)
{
  auto cyl = std::make_shared<Cylinder>(center, dir.normalized(), radius, length,
                                        oid++, mid);
  cyl->movable = movable;
  return cyl;
}

std::vector<HittablePtr>
make_beam_source(const Vec3 &center, double radius, const Vec3 &color,
                 int &oid, int &mid, std::vector<Material> &materials,
                 bool movable)
{
  std::vector<HittablePtr> res;
  // inner sphere
  auto s1 = std::make_shared<Sphere>(center, radius, oid++, mid);
  s1->movable = movable;
  materials.emplace_back();
  materials.back().color = color;
  materials.back().base_color = color;
  materials.back().alpha = 1.0;
  res.push_back(s1);
  ++mid;
  // middle sphere
  double r2 = radius * 1.33;
  Vec3 mix = (color * 0.5) + Vec3(1, 1, 1) * 0.5;
  auto s2 = std::make_shared<Sphere>(center, r2, oid++, mid);
  s2->movable = movable;
  materials.emplace_back();
  materials.back().color = mix;
  materials.back().base_color = mix;
  materials.back().alpha = 0.33;
  res.push_back(s2);
  ++mid;
  // outer sphere
  double r3 = r2 * 1.33;
  auto s3 = std::make_shared<Sphere>(center, r3, oid++, mid);
  s3->movable = movable;
  materials.emplace_back();
  Vec3 white(1, 1, 1);
  materials.back().color = white;
  materials.back().base_color = white;
  materials.back().alpha = 0.67;
  res.push_back(s3);
  ++mid;
  return res;
}

} // namespace rt
