#include "rt/Collision.hpp"
#include <algorithm>
#include <cmath>

namespace rt
{

// ===== BroadPhaseBVH =====

void BroadPhaseBVH::build(const std::vector<AABB> &aabbs,
                          const std::vector<int> &indices)
{
  nodes.clear();
  refAABBs = &aabbs;
  if (indices.empty())
  {
    return;
  }
  std::vector<int> idx = indices;
  build_recursive(idx, 0, idx.size());
}

int BroadPhaseBVH::build_recursive(std::vector<int> &idx, size_t start,
                                   size_t end)
{
  int nodeIdx = static_cast<int>(nodes.size());
  nodes.push_back(Node());
  if (end - start == 1)
  {
    nodes[nodeIdx].object = idx[start];
    nodes[nodeIdx].box = (*refAABBs)[idx[start]];
    return nodeIdx;
  }
  int axis = choose_axis(idx, start, end);
  auto comparator = [axis, this](int a, int b)
  {
    const AABB &boxA = (*refAABBs)[a];
    const AABB &boxB = (*refAABBs)[b];
    double ca = (axis == 0 ? boxA.min.x + boxA.max.x
                           : (axis == 1 ? boxA.min.y + boxA.max.y
                                        : boxA.min.z + boxA.max.z));
    double cb = (axis == 0 ? boxB.min.x + boxB.max.x
                           : (axis == 1 ? boxB.min.y + boxB.max.y
                                        : boxB.min.z + boxB.max.z));
    return ca < cb;
  };
  size_t mid = start + (end - start) / 2;
  std::nth_element(idx.begin() + start, idx.begin() + mid, idx.begin() + end,
                   comparator);
  int left = build_recursive(idx, start, mid);
  int right = build_recursive(idx, mid, end);
  nodes[nodeIdx].left = left;
  nodes[nodeIdx].right = right;
  nodes[nodeIdx].box =
      AABB::surrounding_box(nodes[left].box, nodes[right].box);
  return nodeIdx;
}

int BroadPhaseBVH::choose_axis(std::vector<int> &idx, size_t start,
                               size_t end) const
{
  double mean[3] = {0, 0, 0};
  double m2[3] = {0, 0, 0};
  size_t n = 0;
  for (size_t i = start; i < end; ++i)
  {
    const AABB &b = (*refAABBs)[idx[i]];
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

void BroadPhaseBVH::query(const AABB &box, std::vector<int> &results,
                          int ignore) const
{
  if (nodes.empty())
  {
    return;
  }
  query_recursive(0, box, results, ignore);
}

void BroadPhaseBVH::query_recursive(int nodeIdx, const AABB &box,
                                    std::vector<int> &res, int ignore) const
{
  const Node &n = nodes[nodeIdx];
  if (!n.box.intersects(box))
  {
    return;
  }
  if (n.object != -1)
  {
    if (n.object != ignore)
    {
      res.push_back(n.object);
    }
  }
  else
  {
    query_recursive(n.left, box, res, ignore);
    query_recursive(n.right, box, res, ignore);
  }
}

// ===== CollisionWorld =====

int CollisionWorld::addCollider(const Collider &c)
{
  int id = static_cast<int>(colliders.size());
  colliders.push_back(c);
  aabbs.emplace_back();
  if (c.type != Collider::PLANE)
  {
    nonPlaneIndices.push_back(id);
  }
  updateCollider(id, c);
  return id;
}

void CollisionWorld::updateCollider(int id, const Collider &c)
{
  colliders[id] = c;
  if (c.type == Collider::SPHERE)
  {
    Vec3 r(c.radius, c.radius, c.radius);
    aabbs[id] = AABB(c.center - r, c.center + r);
  }
  else if (c.type == Collider::BOX)
  {
    aabbs[id] = AABB(c.center - c.halfExtents, c.center + c.halfExtents);
  }
  else
  {
    aabbs[id] = AABB();
  }
}

void CollisionWorld::rebuild_bvh()
{
  bvh.build(aabbs, nonPlaneIndices);
}

std::vector<BroadPhasePair> CollisionWorld::getPotentialPairs()
{
  rebuild_bvh();
  std::vector<BroadPhasePair> pairs;
  std::vector<int> results;
  for (size_t i = 0; i < nonPlaneIndices.size(); ++i)
  {
    int idx = nonPlaneIndices[i];
    results.clear();
    bvh.query(aabbs[idx], results, idx);
    for (int j : results)
    {
      if (j > idx)
      {
        pairs.push_back({idx, j});
      }
    }
  }
  return pairs;
}

std::vector<int> CollisionWorld::queryPotentialCameraOverlaps(
    const Collider &cam)
{
  Vec3 r(cam.radius, cam.radius, cam.radius);
  AABB camBox(cam.center - r, cam.center + r);
  std::vector<int> results;
  bvh.query(camBox, results, -1);
  return results;
}

// ===== Narrow phase =====

bool sphere_vs_sphere(const Collider &a, const Collider &b, Contact &c)
{
  Vec3 delta = b.center - a.center;
  double dist2 = Vec3::dot(delta, delta);
  double r = a.radius + b.radius;
  if (dist2 >= r * r)
  {
    return false;
  }
  double dist = std::sqrt(dist2);
  c.hit = true;
  c.depth = r - dist;
  c.normal = dist > 1e-8 ? delta / dist : Vec3{1, 0, 0};
  c.point = a.center + c.normal * a.radius;
  return true;
}

bool sphere_vs_plane(const Collider &sphere, const Collider &plane, Contact &c)
{
  double dist = Vec3::dot(sphere.center, plane.normal) - plane.offset;
  double penetration = sphere.radius - std::abs(dist);
  if (penetration <= 0)
  {
    return false;
  }
  c.hit = true;
  c.depth = penetration;
  c.normal = (dist > 0 ? -plane.normal : plane.normal);
  Vec3 closest = sphere.center - plane.normal * dist;
  c.point = closest;
  return true;
}

bool sphere_vs_box(const Collider &sphere, const Collider &box, Contact &c)
{
  Vec3 min = box.center - box.halfExtents;
  Vec3 max = box.center + box.halfExtents;
  double cx = std::max(min.x, std::min(sphere.center.x, max.x));
  double cy = std::max(min.y, std::min(sphere.center.y, max.y));
  double cz = std::max(min.z, std::min(sphere.center.z, max.z));
  Vec3 closest{cx, cy, cz};
  Vec3 diff = sphere.center - closest;
  double dist2 = Vec3::dot(diff, diff);
  if (dist2 >= sphere.radius * sphere.radius)
  {
    return false;
  }
  double dist = std::sqrt(dist2);
  c.hit = true;
  c.depth = sphere.radius - dist;
  c.normal = dist > 1e-8 ? diff / dist : Vec3{1, 0, 0};
  c.point = closest;
  return true;
}

bool box_vs_plane(const Collider &box, const Collider &plane, Contact &c)
{
  AABB b(box.center - box.halfExtents, box.center + box.halfExtents);
  Vec3 cpt = (b.min + b.max) * 0.5;
  Vec3 e = (b.max - b.min) * 0.5;
  double dist = Vec3::dot(cpt, plane.normal) - plane.offset;
  double r = std::abs(e.x * plane.normal.x) + std::abs(e.y * plane.normal.y) +
             std::abs(e.z * plane.normal.z);
  double penetration = r - std::abs(dist);
  if (penetration <= 0)
  {
    return false;
  }
  c.hit = true;
  c.depth = penetration;
  c.normal = (dist > 0 ? -plane.normal : plane.normal);
  c.point = cpt - plane.normal * dist;
  return true;
}

bool detect_collision(const Collider &a, const Collider &b, Contact &c)
{
  if (a.type == Collider::SPHERE && b.type == Collider::SPHERE)
  {
    return sphere_vs_sphere(a, b, c);
  }
  if (a.type == Collider::SPHERE && b.type == Collider::PLANE)
  {
    return sphere_vs_plane(a, b, c);
  }
  if (a.type == Collider::PLANE && b.type == Collider::SPHERE)
  {
    bool hit = sphere_vs_plane(b, a, c);
    if (hit)
    {
      c.normal = -c.normal; // from plane to sphere -> sphere to plane
    }
    return hit;
  }
  if (a.type == Collider::SPHERE && b.type == Collider::BOX)
  {
    return sphere_vs_box(a, b, c);
  }
  if (a.type == Collider::BOX && b.type == Collider::SPHERE)
  {
    bool hit = sphere_vs_box(b, a, c);
    if (hit)
    {
      c.normal = -c.normal;
      c.point = c.point; // same
    }
    return hit;
  }
  if (a.type == Collider::BOX && b.type == Collider::PLANE)
  {
    return box_vs_plane(a, b, c);
  }
  if (a.type == Collider::PLANE && b.type == Collider::BOX)
  {
    bool hit = box_vs_plane(b, a, c);
    if (hit)
    {
      c.normal = -c.normal;
    }
    return hit;
  }
  return false;
}

} // namespace rt

