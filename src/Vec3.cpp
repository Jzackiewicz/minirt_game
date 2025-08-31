#include "rt/Vec3.hpp"
#include <cmath>

namespace rt
{
Vec3::Vec3() : x(0), y(0), z(0) {}

Vec3::Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

double Vec3::length() const { return std::sqrt(x * x + y * y + z * z); }

double Vec3::length2() const { return x * x + y * y + z * z; }

double Vec3::length_squared() const { return x * x + y * y + z * z; }

Vec3 Vec3::operator+(const Vec3 &b) const
{
  return {x + b.x, y + b.y, z + b.z};
}

Vec3 Vec3::operator-(const Vec3 &b) const
{
  return {x - b.x, y - b.y, z - b.z};
}

Vec3 Vec3::operator-() const { return {-x, -y, -z}; }

Vec3 Vec3::operator*(double s) const { return {x * s, y * s, z * s}; }

Vec3 Vec3::operator/(double s) const { return {x / s, y / s, z / s}; }

Vec3 &Vec3::operator+=(const Vec3 &b)
{
  x += b.x;
  y += b.y;
  z += b.z;
  return *this;
}

Vec3 &Vec3::operator*=(double s)
{
  x *= s;
  y *= s;
  z *= s;
  return *this;
}

double Vec3::dot(const Vec3 &a, const Vec3 &b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Vec3::cross(const Vec3 &a, const Vec3 &b)
{
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 Vec3::normalized() const
{
  double len = std::sqrt(x * x + y * y + z * z);
  if (len == 0)
  {
    return *this;
  }
  return {x / len, y / len, z / len};
}

Vec3 operator*(double s, const Vec3 &v) { return v * s; }

} // namespace rt
