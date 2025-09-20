#include "material.hpp"
#include "Scene.hpp"
#include "Laser.hpp"
#include "Texture.hpp"
#include <algorithm>
#include <cmath>

Vec3 Material::sample_texture(double u, double v) const
{
        if (!texture)
                return color;
        return texture->sample(u, v);
}

Vec3 Material::surface_color(const Scene &scene, const HitRecord &rec) const
{
        Vec3 base = base_color;
        Vec3 current = color;
        if (rec.object_id >= 0 &&
                rec.object_id < static_cast<int>(scene.objects.size()))
        {
                auto obj = scene.objects[rec.object_id];
                if (obj && obj->is_beam())
                {
                        if (auto beam = std::static_pointer_cast<Laser>(obj))
                        {
                                base = current = beam->color;
                        }
                }
        }
        if (checkered)
        {
                Vec3 inv = Vec3(1.0, 1.0, 1.0) - base;
                int chk = (static_cast<int>(std::floor(rec.p.x * 5)) +
                                   static_cast<int>(std::floor(rec.p.y * 5)) +
                                   static_cast<int>(std::floor(rec.p.z * 5))) &
                                  1;
                return chk ? base : inv;
        }
        if (texture)
                return texture->sample(rec.u, rec.v);
        return current;
}

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
                if (L.beam_spotlight && L.spot_radius > 0.0)
                {
                        Vec3 axis_dir = L.direction;
                        double len2 = axis_dir.length_squared();
                        if (len2 <= 1e-12)
                                continue;
                        axis_dir = axis_dir / std::sqrt(len2);
                        Vec3 rel = p - L.position;
                        double axial = Vec3::dot(rel, axis_dir);
                        if (axial < 0.0)
                                continue;
                        if (L.range > 0.0 && axial > L.range)
                                continue;
                        Vec3 closest = L.position + axis_dir * axial;
                        Vec3 radial = p - closest;
                        if (radial.length_squared() > L.spot_radius * L.spot_radius)
                                continue;
                        dist = axial;
                        if (dist <= 1e-6)
                                continue;
                        ldir = axis_dir * -1.0;
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
                if (diff <= 1e-6)
                        continue;
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
