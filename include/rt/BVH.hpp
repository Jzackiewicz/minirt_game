
#pragma once
#include "Hittable.hpp"
#include <limits>
#include <random>
#include <vector>

namespace rt
{
struct BVHNode : public Hittable
{
  HittablePtr left;
  HittablePtr right;
  AABB box;

  BVHNode();
  BVHNode(std::vector<HittablePtr> &objects, size_t start, size_t end);

  bool hit(const Ray &r, double tmin, double tmax,
           HitRecord &rec) const override;
  bool bounding_box(AABB &out) const override;
  bool is_bvh() const override { return true; }

  // Collect all objects whose bounding boxes overlap the given AABB. The
  // optional `ignore` pointer skips a specific object (typically the query
  // object itself).
  void query(const AABB &target, std::vector<HittablePtr> &out,
             const Hittable *ignore = nullptr) const;

private:
  static int choose_axis(std::vector<HittablePtr> &objs, size_t start,
                         size_t end);
};

} // namespace rt
