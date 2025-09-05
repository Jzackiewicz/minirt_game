#pragma once
#include <algorithm>
#include <cmath>

namespace rt
{
class Vec3
{
public:
double x;
double y;
double z;

// Default constructor initializes vector to zero.
Vec3();

// Construct a vector with explicit components.
Vec3(double x_value, double y_value, double z_value);

// Compute length of the vector.
double length() const;

// Compute squared length of the vector.
double length_squared() const;

// Alias for legacy usage.
double length2() const;

Vec3 operator+(const Vec3 &rhs) const;
Vec3 operator-(const Vec3 &rhs) const;
Vec3 operator*(double scalar) const;
Vec3 operator/(double scalar) const;
Vec3 &operator+=(const Vec3 &rhs);
Vec3 &operator*=(double scalar);

static double dot(const Vec3 &a, const Vec3 &b);
static Vec3 cross(const Vec3 &a, const Vec3 &b);
Vec3 normalized() const;
};

Vec3 operator*(double scalar, const Vec3 &vector);

using Color = Vec3;
} // namespace rt
