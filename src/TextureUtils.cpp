#include "TextureUtils.hpp"
#include "Scene.hpp"
#include "material.hpp"
#include "Laser.hpp"
#include <algorithm>
#include <cmath>

Vec3 sample_surface_color(const Scene &scene, const HitRecord &rec, const Material &mat)
{
        Vec3 base = mat.base_color;
        Vec3 color = mat.color;
        if (rec.object_id >= 0 &&
                rec.object_id < static_cast<int>(scene.objects.size()))
        {
                auto obj = scene.objects[rec.object_id];
                if (obj->is_beam())
                {
                        auto beam = std::static_pointer_cast<Laser>(obj);
                        base = color = beam->color;
                }
        }
        if (mat.checkered)
        {
                Vec3 inv = Vec3(1.0, 1.0, 1.0) - base;
                int chk = (static_cast<int>(std::floor(rec.p.x * 5)) +
                                   static_cast<int>(std::floor(rec.p.y * 5)) +
                                   static_cast<int>(std::floor(rec.p.z * 5))) &
                                  1;
                return chk ? base : inv;
        }
        if (mat.texture && mat.texture->is_valid() && rec.has_uv)
        {
                Vec3 sampled = mat.texture->sample(rec.u, rec.v);
                return sampled;
        }
        return color;
}

double compute_effective_alpha(const Material &mat, const HitRecord &rec)
{
        double alpha = mat.alpha;
        if (mat.random_alpha)
        {
                double tpos = std::clamp(rec.beam_ratio, 0.0, 1.0);
                alpha *= (1.0 - tpos);
        }
        return std::clamp(alpha, 0.0, 1.0);
}

