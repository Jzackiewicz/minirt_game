#include "Hittable.hpp"

namespace rt
{
void HitRecord::set_face_normal(const Ray &r, const Vec3 &outward_normal)
{
  front_face = Vec3::dot(r.dir, outward_normal) < 0;
  normal = front_face ? outward_normal : outward_normal * -1.0;
}

} // namespace rt
