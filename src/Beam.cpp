#include "Beam.hpp"

Beam::Beam(const Vec3 &origin, const Vec3 &dir, double ray_radius,
                  double length, double intensity, int base_oid, int laser_mat,
                  int big_mat, int mid_mat, int small_mat, bool enable_laser)
{
       light = std::make_shared<LightRay>(origin, dir, ray_radius, intensity);
       int source_oid = base_oid;
       if (enable_laser)
       {
               laser = std::make_shared<Laser>(
                       origin, dir, length, intensity, base_oid, laser_mat);
               source_oid = base_oid + 1;
       }
       source = std::make_shared<BeamSource>(
               origin, dir, laser, ray_radius, source_oid, big_mat, mid_mat,
               small_mat);
       if (laser)
               laser->source = source;
}
