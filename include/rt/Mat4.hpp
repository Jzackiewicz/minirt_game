#pragma once
#include "Vec3.hpp"
#include <cmath>

namespace rt {

struct Mat4 {
  double m[4][4];
  Mat4();
  static Mat4 identity();
  static Mat4 translate(const Vec3 &t);
  static Mat4 scale(const Vec3 &s);
  static Mat4 rotate(const Vec3 &axis, double angle);
  static Mat4 rotate_from_to(const Vec3 &from, const Vec3 &to);
  Mat4 operator*(const Mat4 &o) const;
  Vec3 apply_point(const Vec3 &p) const;
  Vec3 apply_vector(const Vec3 &v) const;
};

}
