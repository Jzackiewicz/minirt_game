#include "material.hpp"
#include <algorithm>
#include <cmath>

Vec3 phong(const Material &m, const Ambient &ambient,
		   const std::vector<PointLight> &lights, const Vec3 &p, const Vec3 &n,
		   const Vec3 &eye)
{
	Vec3 base = m.base_color;
	Vec3 col = m.color;
	if (m.checkered)
	{
		Vec3 inv = Vec3(1.0, 1.0, 1.0) - base;
		int chk = (static_cast<int>(std::floor(p.x * 5)) +
				   static_cast<int>(std::floor(p.y * 5)) +
				   static_cast<int>(std::floor(p.z * 5))) &
				  1;
		col = chk ? base : inv;
	}
	Vec3 c(0, 0, 0);
	c = Vec3(col.x * ambient.color.x * ambient.intensity,
			 col.y * ambient.color.y * ambient.intensity,
			 col.z * ambient.color.z * ambient.intensity);
        for (const auto &L : lights)
        {
                Vec3 ldir;
                double dist = 0.0;
                if (L.beam_spotlight)
                {
                        Vec3 axis;
                        Vec3 radial;
                        double axial = 0.0;
                        if (!beam_light_geometry(L, p, axis, axial, radial))
                                continue;
                        ldir = axis * -1.0;
                        dist = axial;
                }
                else
                {
                        Vec3 to_light = L.position - p;
                        dist = to_light.length();
                        if (dist <= 1e-6)
                                continue;
                        if (L.range > 0.0 && dist > L.range)
                                continue;
                        ldir = to_light / dist;
                        if (L.cutoff_cos > -1.0)
                        {
                                Vec3 spot_dir = (p - L.position).normalized();
                                if (Vec3::dot(L.direction, spot_dir) < L.cutoff_cos)
                                        continue;
                        }
                }
                double atten = 1.0;
                if (L.range > 0.0)
                {
                        atten = std::max(0.0, 1.0 - dist / L.range);
                        if (atten <= 0.0)
                                continue;
                }
                double diff = std::max(0.0, Vec3::dot(n, ldir));
                Vec3 h = (ldir + eye).normalized();
                double spec = std::pow(std::max(0.0, Vec3::dot(n, h)), m.specular_exp) *
                                          m.specular_k;
                c += Vec3(col.x * L.color.x * L.intensity * diff * atten +
                                          L.color.x * spec * atten,
                                  col.y * L.color.y * L.intensity * diff * atten +
                                          L.color.y * spec * atten,
                                  col.z * L.color.z * L.intensity * diff * atten +
                                          L.color.z * spec * atten);
        }
        return c;
}
