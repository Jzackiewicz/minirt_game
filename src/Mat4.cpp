#include "rt/Mat4.hpp"

namespace rt {

Mat4::Mat4() {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      m[i][j] = 0.0;
}

Mat4 Mat4::identity() {
  Mat4 r;
  for (int i = 0; i < 4; ++i)
    r.m[i][i] = 1.0;
  return r;
}

Mat4 Mat4::translate(const Vec3 &t) {
  Mat4 r = Mat4::identity();
  r.m[0][3] = t.x;
  r.m[1][3] = t.y;
  r.m[2][3] = t.z;
  return r;
}

Mat4 Mat4::scale(const Vec3 &s) {
  Mat4 r = Mat4::identity();
  r.m[0][0] = s.x;
  r.m[1][1] = s.y;
  r.m[2][2] = s.z;
  return r;
}

Mat4 Mat4::rotate(const Vec3 &axis, double angle) {
  Vec3 a = axis.normalized();
  double c = std::cos(angle);
  double s = std::sin(angle);
  double t = 1.0 - c;
  Mat4 r = Mat4::identity();
  r.m[0][0] = c + a.x * a.x * t;
  r.m[0][1] = a.x * a.y * t - a.z * s;
  r.m[0][2] = a.x * a.z * t + a.y * s;
  r.m[1][0] = a.y * a.x * t + a.z * s;
  r.m[1][1] = c + a.y * a.y * t;
  r.m[1][2] = a.y * a.z * t - a.x * s;
  r.m[2][0] = a.z * a.x * t - a.y * s;
  r.m[2][1] = a.z * a.y * t + a.x * s;
  r.m[2][2] = c + a.z * a.z * t;
  return r;
}

Mat4 Mat4::rotate_from_to(const Vec3 &from, const Vec3 &to) {
  Vec3 f = from.normalized();
  Vec3 tvec = to.normalized();
  double cosTheta = Vec3::dot(f, tvec);
  if (cosTheta > 0.9999)
    return Mat4::identity();
  if (cosTheta < -0.9999) {
    Vec3 axis = Vec3::cross(Vec3(1, 0, 0), f);
    if (axis.length_squared() < 1e-6)
      axis = Vec3::cross(Vec3(0, 1, 0), f);
    axis = axis.normalized();
    return Mat4::rotate(axis, M_PI);
  }
  Vec3 axis = Vec3::cross(f, tvec);
  double angle = std::acos(cosTheta);
  return Mat4::rotate(axis, angle);
}

Mat4 Mat4::operator*(const Mat4 &o) const {
  Mat4 r;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        r.m[i][j] += m[i][k] * o.m[k][j];
  return r;
}

Vec3 Mat4::apply_point(const Vec3 &p) const {
  double x = m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3];
  double y = m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3];
  double z = m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[2][3];
  double w = m[3][0] * p.x + m[3][1] * p.y + m[3][2] * p.z + m[3][3];
  if (w != 0.0 && w != 1.0) {
    x /= w;
    y /= w;
    z /= w;
  }
  return Vec3(x, y, z);
}

Vec3 Mat4::apply_vector(const Vec3 &v) const {
  double x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
  double y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
  double z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
  return Vec3(x, y, z);
}

} // namespace rt

