
#pragma once
#include <algorithm>
#include <cmath>

namespace rt
{
struct Vec3
{
  double x;
  double y;
  double z;

  Vec3();
  Vec3(double x_, double y_, double z_);

  double length() const;
  double length2() const;
  double length_squared() const;

  Vec3 operator+(const Vec3 &b) const;
  Vec3 operator-(const Vec3 &b) const;
  Vec3 operator*(double s) const;
  Vec3 operator/(double s) const;
  Vec3 &operator+=(const Vec3 &b);
  Vec3 &operator*=(double s);

  static double dot(const Vec3 &a, const Vec3 &b);
  static Vec3 cross(const Vec3 &a, const Vec3 &b);
  Vec3 normalized() const;
};

Vec3 operator*(double s, const Vec3 &v);

using Color = Vec3;

} // namespace rt
