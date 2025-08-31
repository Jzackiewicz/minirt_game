#include "rt/Collision.hpp"
#include <cassert>
#include <iostream>

using namespace rt;

int main()
{
  // Sphere-Sphere
  Collider s1{Collider::SPHERE};
  s1.center = Vec3(0, 0, 0);
  s1.radius = 1.0;
  Collider s2{Collider::SPHERE};
  s2.center = Vec3(1.5, 0, 0);
  s2.radius = 1.0;
  Contact c;
  bool hit = detect_collision(s1, s2, c);
  assert(hit);
  assert(std::abs(c.depth - 0.5) < 1e-6);
  assert(std::abs(c.normal.x - 1.0) < 1e-6);

  // Sphere-Plane
  Collider plane{Collider::PLANE};
  plane.normal = Vec3(0, 1, 0);
  plane.offset = 0.0;
  Collider s3{Collider::SPHERE};
  s3.center = Vec3(0, 0.5, 0);
  s3.radius = 1.0;
  hit = detect_collision(s3, plane, c);
  assert(hit);
  assert(std::abs(c.depth - 0.5) < 1e-6);
  assert(std::abs(c.normal.y + 1.0) < 1e-6); // normal from sphere to plane

  // Sphere-Box
  Collider box{Collider::BOX};
  box.center = Vec3(0, 0, 0);
  box.halfExtents = Vec3(1, 1, 1);
  Collider s4{Collider::SPHERE};
  s4.center = Vec3(1.5, 0, 0);
  s4.radius = 1.0;
  hit = detect_collision(s4, box, c);
  assert(hit);
  assert(std::abs(c.depth - 0.5) < 1e-6);

  // Box-Plane
  Collider box2{Collider::BOX};
  box2.center = Vec3(0, 0.5, 0);
  box2.halfExtents = Vec3(1, 1, 1);
  hit = detect_collision(box2, plane, c);
  assert(hit);
  assert(std::abs(c.depth - 0.5) < 1e-6);

  std::cout << "All collision tests passed\n";
  return 0;
}

