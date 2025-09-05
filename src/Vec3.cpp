#include "Vec3.hpp"
#include <cmath>

Vec3::Vec3()
{
	x = 0;
	y = 0;
	z = 0;
}

Vec3::Vec3(double x_value, double y_value, double z_value)
{
	x = x_value;
	y = y_value;
	z = z_value;
}

double Vec3::length() const
{
	double result;
	result = std::sqrt(x * x + y * y + z * z);
	return result;
}

double Vec3::length2() const
{
	double result;
	result = x * x + y * y + z * z;
	return result;
}

double Vec3::length_squared() const
{
	double result;
	result = x * x + y * y + z * z;
	return result;
}

Vec3 Vec3::operator+(const Vec3 &rhs) const
{
	Vec3 sum;
	sum.x = x + rhs.x;
	sum.y = y + rhs.y;
	sum.z = z + rhs.z;
	return sum;
}

Vec3 Vec3::operator-(const Vec3 &rhs) const
{
	Vec3 difference;
	difference.x = x - rhs.x;
	difference.y = y - rhs.y;
	difference.z = z - rhs.z;
	return difference;
}

Vec3 Vec3::operator*(double scalar) const
{
	Vec3 product;
	product.x = x * scalar;
	product.y = y * scalar;
	product.z = z * scalar;
	return product;
}

Vec3 Vec3::operator/(double scalar) const
{
	Vec3 quotient;
	quotient.x = x / scalar;
	quotient.y = y / scalar;
	quotient.z = z / scalar;
	return quotient;
}

Vec3 &Vec3::operator+=(const Vec3 &rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	return *this;
}

Vec3 &Vec3::operator*=(double scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	return *this;
}

double Vec3::dot(const Vec3 &a, const Vec3 &b)
{
	double result;
	result = a.x * b.x + a.y * b.y + a.z * b.z;
	return result;
}

Vec3 Vec3::cross(const Vec3 &a, const Vec3 &b)
{
	Vec3 cross_product;
	cross_product.x = a.y * b.z - a.z * b.y;
	cross_product.y = a.z * b.x - a.x * b.z;
	cross_product.z = a.x * b.y - a.y * b.x;
	return cross_product;
}

Vec3 Vec3::normalized() const
{
	double len;
	len = std::sqrt(x * x + y * y + z * z);
	if (len == 0)
	{
		return *this;
	}
	Vec3 result;
	result.x = x / len;
	result.y = y / len;
	result.z = z / len;
	return result;
}

Vec3 operator*(double scalar, const Vec3 &vector)
{
	Vec3 product;
	product = vector * scalar;
	return product;
}
