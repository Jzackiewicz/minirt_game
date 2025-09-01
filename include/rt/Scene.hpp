
#pragma once
#include "BVH.hpp"
#include "Hittable.hpp"
#include "Camera.hpp"
#include "light.hpp"
#include "material.hpp"
#include <memory>
#include <vector>

namespace rt
{
struct Scene
{
  std::vector<HittablePtr> objects;
  std::vector<PointLight> lights;
  Ambient ambient{Vec3(1, 1, 1), 0.0};
  std::shared_ptr<Hittable> accel;

  void update_beams(const std::vector<Material> &mats);
  void build_bvh();
  bool hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const;
  bool collides(int index) const;
  Vec3 move_with_collision(int index, const Vec3 &delta);
  Vec3 move_camera(Camera &cam, const Vec3 &delta,
                   const std::vector<Material> &mats);
};

} // namespace rt
