#include "rt/Renderer.h"
#include <thread>
#include <atomic>
#include <fstream>
#include <cmath>
#include <algorithm>

namespace rt {

static bool in_shadow(const Scene& scene, const Vec3& p, const Vec3& light_pos) {
    Vec3 dir = (light_pos - p).normalized();
    Ray shadow_ray(p + dir*1e-4, dir);
    HitRecord tmp;
    double dist_to_light = (light_pos - p).length();
    return scene.hit(shadow_ray, 1e-4, dist_to_light - 1e-4, tmp);
}

Renderer::Renderer(const Scene& s, const Camera& c)
    : scene(s), cam(c) {}

void Renderer::render_ppm(const std::string& path,
                          const std::vector<Material>& mats,
                          const RenderSettings& rset)
{
    const int W = rset.width;
    const int H = rset.height;
    const int T = (rset.threads > 0)
                    ? rset.threads
                    : (std::thread::hardware_concurrency()
                        ? (int)std::thread::hardware_concurrency()
                        : 8);

    std::vector<Vec3> framebuffer(W * H);
    std::atomic<int> next_row{0};

    auto worker = [&]() {
        HitRecord rec;
        for (;;) {
            int y = next_row.fetch_add(1);
            if (y >= H) break;
            for (int x = 0; x < W; ++x) {
                double u = (x + 0.5) / W;
                double v = (y + 0.5) / H;
                Ray r = cam.ray_through(u, v);
                Vec3 col(0,0,0);
                if (scene.hit(r, 1e-4, 1e9, rec)) {
                    const Material& m = mats[rec.material_id];
                    Vec3 eye = (cam.origin - rec.p).normalized();
                    Vec3 base = m.color;
                    Vec3 sum( base.x*scene.ambient.color.x * scene.ambient.intensity,
                              base.y*scene.ambient.color.y * scene.ambient.intensity,
                              base.z*scene.ambient.color.z * scene.ambient.intensity );
                    for (const auto& L : scene.lights) {
                        if (in_shadow(scene, rec.p, L.position)) continue;
                        Vec3 ldir = (L.position - rec.p).normalized();
                        double diff = std::max(0.0, Vec3::dot(rec.normal, ldir));
                        Vec3 h = (ldir + eye).normalized();
                        double spec = std::pow(std::max(0.0, Vec3::dot(rec.normal, h)), m.specular_exp) * m.specular_k;
                        sum += Vec3(base.x*L.color.x*L.intensity*diff + L.color.x*spec,
                                    base.y*L.color.y*L.intensity*diff + L.color.y*spec,
                                    base.z*L.color.z*L.intensity*diff + L.color.z*spec);
                    }
                    col = sum;
                } else {
                    col = Vec3(0.0, 0.0, 0.0);
                }
                framebuffer[y * W + x] = col;
            }
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(T);
    for (int i = 0; i < T; ++i) pool.emplace_back(worker);
    for (auto& th : pool) th.join();

    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << W << " " << H << "\n255\n";
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            Vec3 c = framebuffer[y * W + x];
            c.x = std::clamp(c.x, 0.0, 1.0);
            c.y = std::clamp(c.y, 0.0, 1.0);
            c.z = std::clamp(c.z, 0.0, 1.0);
            unsigned char r = static_cast<unsigned char>(std::lround(c.x * 255.0));
            unsigned char g = static_cast<unsigned char>(std::lround(c.y * 255.0));
            unsigned char b = static_cast<unsigned char>(std::lround(c.z * 255.0));
            out.put(r).put(g).put(b);
        }
    }
}

} // namespace rt
