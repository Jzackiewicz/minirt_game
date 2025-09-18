#include "Beam.hpp"

Beam::Beam(const Vec3 &origin, const Vec3 &dir, double ray_radius,
                  double length, double intensity, int base_oid, int laser_mat,
                  int big_mat, int mid_mat, int small_mat, bool with_laser,
                  const Vec3 &color)
{
        light = std::make_shared<LightRay>(origin, dir, ray_radius, intensity, color);
        if (with_laser)
        {
                laser = std::make_shared<Laser>(origin, dir, length, intensity,
                                                                                base_oid, laser_mat, 0.0, -1.0,
                                                                                ray_radius);
               laser->color = color;
               source = std::make_shared<BeamSource>(origin, dir, laser, light,
                                                                                ray_radius, base_oid + 1,
                                                                                big_mat, mid_mat, small_mat);
               laser->source = source;
       }
       else
       {
               source = std::make_shared<BeamSource>(origin, dir, nullptr, light,
                                                                                ray_radius, base_oid, big_mat,
                                                                                mid_mat, small_mat);
       }
}
