#pragma once
#include "Vec3.hpp"

namespace rt {
struct Mat4 {
  double m[4][4];
  Mat4();
  static Mat4 identity();
  static Mat4 translation(const Vec3 &t);
  static Mat4 scaling(const Vec3 &s);
  static Mat4 rotation_axis_angle(const Vec3 &axis, double angle);
  static Mat4 rotation_from_to(const Vec3 &from, const Vec3 &to);
  Mat4 operator*(const Mat4 &o) const;
  Vec3 transform_point(const Vec3 &p) const;
  Vec3 transform_vector(const Vec3 &v) const;
};
}
