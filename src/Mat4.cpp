#include "rt/Mat4.hpp"
#include <cmath>
#include <algorithm>

namespace rt {
Mat4::Mat4() {
  for (int i=0;i<4;++i)
    for (int j=0;j<4;++j)
      m[i][j] = 0.0;
}

Mat4 Mat4::identity() {
  Mat4 r;
  for (int i=0;i<4;++i)
    r.m[i][i] = 1.0;
  return r;
}

Mat4 Mat4::translation(const Vec3 &t) {
  Mat4 r = identity();
  r.m[0][3] = t.x;
  r.m[1][3] = t.y;
  r.m[2][3] = t.z;
  return r;
}

Mat4 Mat4::scaling(const Vec3 &s) {
  Mat4 r = identity();
  r.m[0][0] = s.x;
  r.m[1][1] = s.y;
  r.m[2][2] = s.z;
  return r;
}

Mat4 Mat4::rotation_axis_angle(const Vec3 &axis, double angle) {
  Vec3 a = axis.normalized();
  double c = std::cos(angle);
  double s = std::sin(angle);
  double t = 1.0 - c;
  Mat4 r = identity();
  r.m[0][0] = t*a.x*a.x + c;
  r.m[0][1] = t*a.x*a.y - s*a.z;
  r.m[0][2] = t*a.x*a.z + s*a.y;
  r.m[1][0] = t*a.x*a.y + s*a.z;
  r.m[1][1] = t*a.y*a.y + c;
  r.m[1][2] = t*a.y*a.z - s*a.x;
  r.m[2][0] = t*a.x*a.z - s*a.y;
  r.m[2][1] = t*a.y*a.z + s*a.x;
  r.m[2][2] = t*a.z*a.z + c;
  return r;
}

Mat4 Mat4::rotation_from_to(const Vec3 &from, const Vec3 &to) {
  Vec3 f = from.normalized();
  Vec3 tvec = to.normalized();
  double dot = Vec3::dot(f, tvec);
  if (dot > 0.999999) {
    return identity();
  }
  if (dot < -0.999999) {
    Vec3 axis = Vec3::cross(Vec3(1,0,0), f);
    if (axis.length_squared() < 1e-12)
      axis = Vec3::cross(Vec3(0,1,0), f);
    axis = axis.normalized();
    return rotation_axis_angle(axis, std::acos(-1.0));
  }
  Vec3 axis = Vec3::cross(f, tvec);
  double angle = std::acos(std::clamp(dot, -1.0, 1.0));
  return rotation_axis_angle(axis, angle);
}

Mat4 Mat4::operator*(const Mat4 &o) const {
  Mat4 r;
  for (int i=0;i<4;++i)
    for (int j=0;j<4;++j)
      for (int k=0;k<4;++k)
        r.m[i][j] += m[i][k]*o.m[k][j];
  return r;
}

Vec3 Mat4::transform_point(const Vec3 &p) const {
  double x = m[0][0]*p.x + m[0][1]*p.y + m[0][2]*p.z + m[0][3];
  double y = m[1][0]*p.x + m[1][1]*p.y + m[1][2]*p.z + m[1][3];
  double z = m[2][0]*p.x + m[2][1]*p.y + m[2][2]*p.z + m[2][3];
  return Vec3(x,y,z);
}

Vec3 Mat4::transform_vector(const Vec3 &v) const {
  double x = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z;
  double y = m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z;
  double z = m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z;
  return Vec3(x,y,z);
}

} // namespace rt
