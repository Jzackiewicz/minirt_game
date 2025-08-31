#pragma once
#include "Vec3.hpp"
#include "AABB.hpp"
#include <vector>
#include <utility>

namespace rt
{
struct Contact
{
  bool hit = false;
  Vec3 normal{0, 0, 0};
  double depth = 0.0;
  Vec3 point{0, 0, 0};
};

struct Collider
{
  enum Type
  {
    SPHERE,
    PLANE,
    BOX
  } type;

  Vec3 center{0, 0, 0};    // sphere & box center
  double radius = 0.0;     // sphere radius
  Vec3 halfExtents{0, 0, 0}; // box half sizes (axis-aligned)

  // Plane data: normal and offset so that dot(n, x) = offset defines the plane
  Vec3 normal{0, 1, 0};
  double offset = 0.0;

  // Inverse mass for simple solver (0 = static)
  double invMass = 0.0;
};

struct BroadPhasePair
{
  int a;
  int b;
};

class BroadPhaseBVH
{
public:
  void build(const std::vector<AABB> &aabbs, const std::vector<int> &indices);
  void query(const AABB &box, std::vector<int> &results, int ignore = -1) const;

private:
  struct Node
  {
    AABB box;
    int left = -1;
    int right = -1;
    int object = -1;
  };
  std::vector<Node> nodes;
  const std::vector<AABB> *refAABBs = nullptr;
  int build_recursive(std::vector<int> &idx, size_t start, size_t end);
  void query_recursive(int nodeIdx, const AABB &box, std::vector<int> &res,
                       int ignore) const;
  int choose_axis(std::vector<int> &idx, size_t start, size_t end) const;
};

class CollisionWorld
{
public:
  int addCollider(const Collider &c);
  void updateCollider(int id, const Collider &c);
  std::vector<BroadPhasePair> getPotentialPairs();
  std::vector<int> queryPotentialCameraOverlaps(const Collider &cam);
  const Collider &getCollider(int id) const { return colliders[id]; }

private:
  std::vector<Collider> colliders;
  std::vector<AABB> aabbs;
  std::vector<int> nonPlaneIndices;
  BroadPhaseBVH bvh;
  void rebuild_bvh();
};

// Narrow phase analytic tests
bool sphere_vs_sphere(const Collider &a, const Collider &b, Contact &c);
bool sphere_vs_plane(const Collider &sphere, const Collider &plane, Contact &c);
bool sphere_vs_box(const Collider &sphere, const Collider &box, Contact &c);
bool box_vs_plane(const Collider &box, const Collider &plane, Contact &c);

bool detect_collision(const Collider &a, const Collider &b, Contact &c);

} // namespace rt

