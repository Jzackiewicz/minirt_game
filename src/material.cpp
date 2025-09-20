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
                Vec3 to_light = L.position - p;
                double dist = to_light.length();
                double axial_dist = 0.0;
                if (!spotlight_covers_point(L, p, &axial_dist))
                        continue;
                if (dist <= 1e-6)
                        continue;
                Vec3 ldir = to_light / dist;
                double atten = 1.0;
                double effective_dist =
                        (L.beam_spotlight && L.spot_radius > 0.0)
                                ? std::max(0.0, axial_dist)
                                : dist;
                if (L.range > 0.0)
                {
                        atten = std::max(0.0, 1.0 - effective_dist / L.range);
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
