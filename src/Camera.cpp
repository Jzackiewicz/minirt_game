#include "Camera.hpp"
#include <algorithm>

namespace rt
{
Camera::Camera(const Vec3 &pos, const Vec3 &look_at, double fov_deg_,
               double aspect_)
    : origin(pos), fov_deg(fov_deg_), aspect(aspect_)
{
  forward = (look_at - pos).normalized();
  Vec3 world_up(0, 1, 0);
  right = Vec3::cross(forward, world_up).normalized();
  up = Vec3::cross(right, forward).normalized();
}

void Camera::move(const Vec3 &delta) { origin += delta; }

void Camera::rotate(double yaw, double pitch)
{
  Vec3 world_up(0, 1, 0);

  // Derive current yaw and pitch from the forward vector so that we can
  // clamp the pitch and rebuild the orientation from scratch. Without this
  // clamp the forward vector can align with the world_up vector which makes
  // the right vector undefined and causes the camera to flip.
  double current_yaw = std::atan2(-forward.x, -forward.z);
  double current_pitch = std::asin(forward.y);

  double max_pitch = 89.0 * M_PI / 180.0;
  double new_pitch = std::clamp(current_pitch + pitch, -max_pitch, max_pitch);
  double new_yaw = current_yaw + yaw;

  double cos_pitch = std::cos(new_pitch);
  forward =
      Vec3(-std::sin(new_yaw) * cos_pitch, std::sin(new_pitch),
           -std::cos(new_yaw) * cos_pitch);

  right = Vec3::cross(forward, world_up).normalized();
  up = Vec3::cross(right, forward).normalized();
}

Ray Camera::ray_through(double u, double v) const
{
  double fov_rad = fov_deg * M_PI / 180.0;
  double half_h = std::tan(fov_rad * 0.5);
  double half_w = aspect * half_h;
  Vec3 dir =
      (forward + (2 * u - 1) * half_w * right + (1 - 2 * v) * half_h * up)
          .normalized();
  return Ray(origin, dir);
}

} // namespace rt
