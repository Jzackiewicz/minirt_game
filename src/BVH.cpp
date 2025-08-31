#include "rt/BVH.hpp"
#include <algorithm>

namespace rt
{
BVHNode::BVHNode() {}

BVHNode::BVHNode(std::vector<HittablePtr> &objects, size_t start, size_t end)
{
  auto axis = choose_axis(objects, start, end);
  auto comparator = [axis](const HittablePtr &a, const HittablePtr &b)
  {
    AABB boxA, boxB;
    a->bounding_box(boxA);
    b->bounding_box(boxB);
    double ca = (axis == 0 ? boxA.min.x + boxA.max.x
                           : (axis == 1 ? boxA.min.y + boxA.max.y
                                        : boxA.min.z + boxA.max.z));
    double cb = (axis == 0 ? boxB.min.x + boxB.max.x
                           : (axis == 1 ? boxB.min.y + boxB.max.y
                                        : boxB.min.z + boxB.max.z));
    return ca < cb;
  };
  size_t object_span = end - start;
  if (object_span == 1)
  {
    left = right = objects[start];
  }
  else if (object_span == 2)
  {
    if (comparator(objects[start], objects[start + 1]))
    {
      left = objects[start];
      right = objects[start + 1];
    }
    else
    {
      left = objects[start + 1];
      right = objects[start];
    }
  }
  else
  {
    std::sort(objects.begin() + start, objects.begin() + end, comparator);
    size_t mid = start + object_span / 2;
    left = std::make_shared<BVHNode>(objects, start, mid);
    right = std::make_shared<BVHNode>(objects, mid, end);
  }
  AABB boxLeft, boxRight;
  left->bounding_box(boxLeft);
  right->bounding_box(boxRight);
  box = AABB::surrounding_box(boxLeft, boxRight);
}

bool BVHNode::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  if (!box.hit(r, tmin, tmax))
  {
    return false;
  }
  bool hitLeft = left->hit(r, tmin, tmax, rec);
  bool hitRight = right->hit(r, tmin, hitLeft ? rec.t : tmax, rec);
  return hitLeft || hitRight;
}

bool BVHNode::bounding_box(AABB &out) const
{
  out = box;
  return true;
}

void BVHNode::query(const AABB &target, std::vector<HittablePtr> &out) const
{
  if (!box.intersects(target))
    return;
  if (left->is_bvh())
    static_cast<BVHNode *>(left.get())->query(target, out);
  else
  {
    AABB lb;
    if (left->bounding_box(lb) && lb.intersects(target))
      out.push_back(left);
  }
  if (right->is_bvh())
    static_cast<BVHNode *>(right.get())->query(target, out);
  else
  {
    AABB rb;
    if (right->bounding_box(rb) && rb.intersects(target))
      out.push_back(right);
  }
}

int BVHNode::choose_axis(std::vector<HittablePtr> &objs, size_t start,
                         size_t end)
{
  double mean[3] = {0, 0, 0};
  double m2[3] = {0, 0, 0};
  size_t n = 0;
  for (size_t i = start; i < end; ++i)
  {
    AABB b;
    objs[i]->bounding_box(b);
    double cx = 0.5 * (b.min.x + b.max.x);
    double cy = 0.5 * (b.min.y + b.max.y);
    double cz = 0.5 * (b.min.z + b.max.z);
    double c[3] = {cx, cy, cz};
    ++n;
    for (int a = 0; a < 3; ++a)
    {
      double delta = c[a] - mean[a];
      mean[a] += delta / n;
      double delta2 = c[a] - mean[a];
      m2[a] += delta * delta2;
    }
  }
  double varx = (n > 1 ? m2[0] / (n - 1) : 0);
  double vary = (n > 1 ? m2[1] / (n - 1) : 0);
  double varz = (n > 1 ? m2[2] / (n - 1) : 0);
  if (varx >= vary && varx >= varz)
  {
    return 0;
  }
  if (vary >= varx && vary >= varz)
  {
    return 1;
  }
  return 2;
}

} // namespace rt
