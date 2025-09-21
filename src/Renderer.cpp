#include "Renderer.hpp"
#include "AABB.hpp"
#include "BeamSource.hpp"
#include "BeamTarget.hpp"
#include "Config.hpp"
#include "Settings.hpp"
#include "MapSaver.hpp"
#include "Parser.hpp"
#include "LevelFinishedMenu.hpp"
#include "PauseMenu.hpp"
#include "Laser.hpp"
#include "Plane.hpp"
#include "Sphere.hpp"
#include "Cube.hpp"
#include "Cone.hpp"
#include "Cylinder.hpp"
#include "CustomCharacter.hpp"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <system_error>
#include <string>
#include <thread>
#include <utility>

static inline Vec3 mix_colors(const Vec3 &a, const Vec3 &b, double alpha)
{
        return a * (1.0 - alpha) + b * alpha;
}

static constexpr double kAltColorAmount = 0.35;
static constexpr double kQuotaScoreEpsilon = 1e-3;

static Vec3 brighten_color(const Vec3 &color)
{
        return Vec3(std::clamp(color.x + (1.0 - color.x) * kAltColorAmount, 0.0, 1.0),
                                std::clamp(color.y + (1.0 - color.y) * kAltColorAmount, 0.0, 1.0),
                                std::clamp(color.z + (1.0 - color.z) * kAltColorAmount, 0.0, 1.0));
}

static Vec3 darken_color(const Vec3 &color)
{
        double factor = 1.0 - kAltColorAmount;
        return Vec3(std::clamp(color.x * factor, 0.0, 1.0),
                                std::clamp(color.y * factor, 0.0, 1.0),
                                std::clamp(color.z * factor, 0.0, 1.0));
}

static Vec3 normalize_or(const Vec3 &v, const Vec3 &fallback = Vec3(0, 0, 1))
{
        double len2 = v.length_squared();
        if (len2 <= 1e-12)
                return fallback;
        return v / std::sqrt(len2);
}

static bool beam_parameters(const PointLight &L, const Vec3 &point, Vec3 &axis_dir,
                           double &axial_dist, double &radial_sq)
{
        if (!L.beam_spotlight || L.spot_radius <= 0.0)
                return false;
        if (L.direction.length_squared() <= 1e-12)
                return false;
        axis_dir = normalize_or(L.direction);
        Vec3 rel = point - L.position;
        axial_dist = Vec3::dot(rel, axis_dir);
        if (axial_dist < 0.0)
                return false;
        if (L.range > 0.0 && axial_dist > L.range)
                return false;
        Vec3 closest = L.position + axis_dir * axial_dist;
        Vec3 radial = point - closest;
        radial_sq = radial.length_squared();
        double radius = L.spot_radius;
        return radial_sq <= radius * radius;
}

static bool beam_light_through(const Scene &scene, const std::vector<Material> &mats,
                               const Vec3 &p, const PointLight &L, const Vec3 &axis_dir,
                               double axis_dist, Vec3 &color, double &intensity)
{
        if (axis_dist <= 1e-6)
        {
                color = L.color;
                intensity = L.intensity;
                return true;
        }
        color = L.color;
        intensity = L.intensity;
        Vec3 dir = axis_dir * -1.0;
        Ray shadow_ray(p + dir * 1e-4, dir);
        double max_dist = axis_dist - 1e-4;
        while (max_dist > 1e-4)
        {
                HitRecord tmp;
                bool hit_any = false;
                double closest = max_dist;
                int hit_mat = -1;
                for (const auto &obj : scene.objects)
                {
                        if (!obj->casts_shadow())
                                continue;
                        if (obj->is_beam())
                                continue;
                        if (std::find(L.ignore_ids.begin(), L.ignore_ids.end(),
                                                  obj->object_id) != L.ignore_ids.end())
                                continue;
                        if (obj->hit(shadow_ray, 1e-4, closest, tmp))
                        {
                                closest = tmp.t;
                                hit_mat = tmp.material_id;
                                hit_any = true;
                        }
                }
                if (!hit_any)
                        break;
                const Material &m = mats[hit_mat];
                if (m.alpha >= 1.0)
                        return false;
                color = mix_colors(color, m.base_color, m.alpha);
                intensity *= (1.0 - m.alpha);
                shadow_ray.orig = shadow_ray.orig + shadow_ray.dir * (closest + 1e-4);
                max_dist -= closest + 1e-4;
                if (intensity <= 1e-4)
                        return false;
        }
        return true;
}

static bool light_through(const Scene &scene, const std::vector<Material> &mats,
                                                const Vec3 &p, const PointLight &L,
                                                Vec3 &color, double &intensity)
{
        Vec3 to_light = L.position - p;
        double dist_to_light = to_light.length();
        if (L.range > 0.0 && dist_to_light > L.range)
                return false;
        Vec3 dir = to_light.normalized();
        Ray shadow_ray(p + dir * 1e-4, dir);
        double max_dist = dist_to_light - 1e-4;
        color = L.color;
        intensity = L.intensity;
        while (max_dist > 1e-4)
        {
                HitRecord tmp;
                bool hit_any = false;
                double closest = max_dist;
                int hit_mat = -1;
                for (const auto &obj : scene.objects)
                {
                        if (!obj->casts_shadow())
                                continue;
                        if (obj->is_beam())
                                continue;
                        if (std::find(L.ignore_ids.begin(), L.ignore_ids.end(),
                                                  obj->object_id) != L.ignore_ids.end())
                                continue;
                        if (obj->hit(shadow_ray, 1e-4, closest, tmp))
                        {
                                closest = tmp.t;
                                hit_mat = tmp.material_id;
                                hit_any = true;
                        }
                }
                if (!hit_any)
                        break;
                const Material &m = mats[hit_mat];
                if (m.alpha >= 1.0)
                        return false;
                color = mix_colors(color, m.base_color, m.alpha);
                intensity *= (1.0 - m.alpha);
                shadow_ray.orig = shadow_ray.orig + shadow_ray.dir * (closest + 1e-4);
                max_dist -= closest + 1e-4;
                if (intensity <= 1e-4)
                        return false;
        }
        return true;
}

namespace
{

struct Basis
{
        Vec3 u;
        Vec3 v;
        Vec3 w;
};

Basis make_basis(const Vec3 &axis)
{
        Basis b{};
        double len2 = axis.length_squared();
        if (len2 <= 1e-12)
        {
                b.w = Vec3(0, 0, 1);
                b.u = Vec3(1, 0, 0);
                b.v = Vec3(0, 1, 0);
                return b;
        }
        b.w = axis / std::sqrt(len2);
        Vec3 helper = (std::abs(b.w.z) < 0.999) ? Vec3(0, 0, 1) : Vec3(0, 1, 0);
        b.u = Vec3::cross(helper, b.w);
        double ulen = b.u.length();
        if (ulen <= 1e-12)
        {
                helper = Vec3(0, 1, 0);
                b.u = Vec3::cross(helper, b.w);
                ulen = b.u.length();
        }
        b.u = (ulen > 1e-12) ? b.u / ulen : Vec3(1, 0, 0);
        b.v = Vec3::cross(b.w, b.u);
        return b;
}

bool light_ignores(const PointLight &L, int object_id)
{
        return std::find(L.ignore_ids.begin(), L.ignore_ids.end(), object_id) !=
               L.ignore_ids.end();
}

Vec3 clamp_color(const Vec3 &c, double lo = 0.0, double hi = 1.0)
{
        return Vec3(std::clamp(c.x, lo, hi), std::clamp(c.y, lo, hi),
                                std::clamp(c.z, lo, hi));
}

Vec3 surface_color_at(const Scene &scene, const HitRecord &rec,
                                         const Material &mat,
                                         bool use_base_state = false)
{
        Vec3 base = mat.base_color;
        Vec3 col = use_base_state ? mat.base_color : mat.color;
        if (rec.object_id >= 0 &&
                rec.object_id < static_cast<int>(scene.objects.size()))
        {
                auto obj = scene.objects[rec.object_id];
                if (obj->is_beam())
                {
                        auto beam = std::static_pointer_cast<Laser>(obj);
                        base = col = beam->color;
                }
        }
        if (mat.has_texture() && rec.has_uv)
        {
                Vec3 tex = mat.texture->sample(rec.u, rec.v);
                col = tex;
        }
        if (!use_base_state && mat.checkered)
        {
                Vec3 brighter = brighten_color(base);
                Vec3 darker = darken_color(base);
                int chk = (static_cast<int>(std::floor(rec.p.x * 5)) +
                                   static_cast<int>(std::floor(rec.p.y * 5)) +
                                   static_cast<int>(std::floor(rec.p.z * 5))) &
                                  1;
                col = chk ? brighter : darker;
        }
        return col;
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

Vec3 ambient_contribution(const Scene &scene, const Vec3 &surface_color)
{
        return Vec3(surface_color.x * scene.ambient.color.x * scene.ambient.intensity,
                                surface_color.y * scene.ambient.color.y * scene.ambient.intensity,
                                surface_color.z * scene.ambient.color.z * scene.ambient.intensity);
}

Vec3 light_contribution(const Scene &scene, const std::vector<Material> &mats,
                                         const PointLight &light, const HitRecord &rec,
                                         const Vec3 &surface_color, const Material &mat,
                                         const Vec3 &point, const Vec3 &view_dir)
{
        if (light_ignores(light, rec.object_id))
                return Vec3(0.0, 0.0, 0.0);
        Vec3 lcolor;
        double lintensity;
        Vec3 ldir;
        double atten = 1.0;
        if (light.beam_spotlight && light.spot_radius > 0.0)
        {
                Vec3 axis_dir;
                double axial_dist;
                double radial_sq;
                if (!beam_parameters(light, point, axis_dir, axial_dist, radial_sq))
                        return Vec3(0.0, 0.0, 0.0);
                if (!beam_light_through(scene, mats, point, light, axis_dir, axial_dist,
                                        lcolor, lintensity))
                        return Vec3(0.0, 0.0, 0.0);
                if (light.range > 0.0)
                {
                        atten = std::max(0.0, 1.0 - axial_dist / light.range);
                        if (atten <= 0.0)
                                return Vec3(0.0, 0.0, 0.0);
                }
                ldir = axis_dir * -1.0;
        }
        else
        {
                Vec3 to_light = light.position - point;
                double dist = to_light.length();
                if (dist <= 1e-6)
                        return Vec3(0.0, 0.0, 0.0);
                ldir = to_light / dist;
                if (light.cutoff_cos > -1.0)
                {
                        Vec3 spot_dir = (point - light.position).normalized();
                        if (Vec3::dot(light.direction, spot_dir) < light.cutoff_cos)
                                return Vec3(0.0, 0.0, 0.0);
                }
                if (!light_through(scene, mats, point, light, lcolor, lintensity))
                        return Vec3(0.0, 0.0, 0.0);
                if (light.range > 0.0)
                {
                        atten = std::max(0.0, 1.0 - dist / light.range);
                        if (atten <= 0.0)
                                return Vec3(0.0, 0.0, 0.0);
                }
        }
        double diff = std::max(0.0, Vec3::dot(rec.normal, ldir));
        if (diff <= 1e-6)
                return Vec3(0.0, 0.0, 0.0);
        Vec3 h = (ldir + view_dir).normalized();
        double spec = 0.0;
        if (mat.specular_k > 0.0)
        {
                spec = std::pow(std::max(0.0, Vec3::dot(rec.normal, h)), mat.specular_exp) *
                       mat.specular_k;
        }
        double diff_term = lintensity * diff * atten;
        return Vec3(surface_color.x * lcolor.x * diff_term + lcolor.x * spec * atten,
                                surface_color.y * lcolor.y * diff_term + lcolor.y * spec * atten,
                                surface_color.z * lcolor.z * diff_term + lcolor.z * spec * atten);
}

double luminance(const Vec3 &c)
{
        return 0.2126 * c.x + 0.7152 * c.y + 0.0722 * c.z;
}

static Vec3 highlight_alternate_color(const Vec3 &base)
{
        double lum = luminance(base);
        bool use_brighter = lum < 0.5;
        return use_brighter ? brighten_color(base) : darken_color(base);
}

double trace_spotlight_sample(const Scene &scene, const std::vector<Material> &mats,
                                                         const PointLight &L, const Vec3 &axis_dir,
                                                         const Vec3 &sample_origin,
                                                         double sample_area, int limit_object = -1)
{
        if (L.intensity <= 1e-4)
                return 0.0;
        Vec3 dir = normalize_or(axis_dir);
        const double max_range = (L.range > 0.0) ? L.range : 1e9;
        double travelled = 0.0;
        Vec3 origin = sample_origin;
        double transmittance = 1.0;
        double total_area = 0.0;

        while (travelled < max_range - 1e-4 && transmittance > 1e-4)
        {
                Ray ray(origin, dir);
                double closest = max_range - travelled;
                HitRecord rec;
                bool hit_any = false;
                Hittable *hit_obj = nullptr;
                for (const auto &obj : scene.objects)
                {
                        if (obj->is_beam())
                                continue;
                        if (light_ignores(L, obj->object_id))
                                continue;
                        HitRecord tmp;
                        if (obj->hit(ray, 1e-4, closest, tmp))
                        {
                                closest = tmp.t;
                                rec = tmp;
                                hit_any = true;
                                hit_obj = obj.get();
                        }
                }
                if (!hit_any)
                        break;

                travelled += closest;
                Vec3 point = ray.at(closest);

                if (hit_obj && hit_obj->scorable && !hit_obj->is_beam() &&
                    (limit_object < 0 || hit_obj->object_id == limit_object))
                {
                        Vec3 ldir = dir * -1.0;
                        double cos_incident =
                                std::max(0.0, Vec3::dot(rec.normal, ldir));
                        if (cos_incident > 1e-6)
                        {
                                const Material &mat = mats[rec.material_id];
                                Vec3 surface_color =
                                        surface_color_at(scene, rec, mat, true);
                                Vec3 view_dir = rec.normal.normalized();
                                Vec3 base = ambient_contribution(scene, surface_color);
                                for (const auto &other : scene.lights)
                                {
                                        if (&other == &L)
                                                continue;
                                        base += light_contribution(scene, mats, other, rec,
                                                                   surface_color, mat, point,
                                                                   view_dir);
                                }
                                Vec3 beam_contrib = light_contribution(
                                        scene, mats, L, rec, surface_color, mat, point,
                                        view_dir);
                                if (beam_contrib.length_squared() > 1e-12)
                                {
                                        Vec3 base_clamped = clamp_color(base);
                                        Vec3 with_clamped =
                                                clamp_color(base + beam_contrib);
                                        Vec3 delta = with_clamped - base_clamped;
                                        delta.x = std::max(0.0, delta.x);
                                        delta.y = std::max(0.0, delta.y);
                                        delta.z = std::max(0.0, delta.z);
                                        double delta_luma = luminance(delta);
                                        if (delta_luma > 1e-6)
                                        {
                                                double area = sample_area / cos_incident;
                                                double alpha = compute_effective_alpha(mat, rec);
                                                total_area += area * delta_luma * alpha;
                                        }
                                }
                        }
                }

                const Material &mat = mats[rec.material_id];
                double effective_alpha = compute_effective_alpha(mat, rec);
                if (effective_alpha >= 1.0)
                        break;

                transmittance *= (1.0 - effective_alpha);
                if (transmittance <= 1e-4)
                        break;

                travelled += 1e-4;
                origin = point + dir * 1e-4;
        }

        return total_area;
}

double integrate_spotlight_area(const Scene &scene, const std::vector<Material> &mats,
                                                           const PointLight &L)
{
        if (!L.beam_spotlight || L.intensity <= 0.0)
                return 0.0;
        if (L.spot_radius <= 0.0)
                return 0.0;
        Basis basis = make_basis(L.direction);
        Vec3 axis_dir = basis.w;
        const int grid = 16;
        double disk_area = M_PI * L.spot_radius * L.spot_radius;
        if (disk_area <= 1e-12)
                return 0.0;
        double sample_area = disk_area / (grid * grid);

        double total = 0.0;
        for (int iy = 0; iy < grid; ++iy)
        {
                for (int ix = 0; ix < grid; ++ix)
                {
                        double su = (ix + 0.5) / static_cast<double>(grid);
                        double sv = (iy + 0.5) / static_cast<double>(grid);
                        double radius = L.spot_radius * std::sqrt(su);
                        double phi = 2.0 * M_PI * sv;
                        Vec3 offset = basis.u * (std::cos(phi) * radius) +
                                      basis.v * (std::sin(phi) * radius);
                        Vec3 sample_origin = L.position + offset + axis_dir * 1e-4;
                        total += trace_spotlight_sample(scene, mats, L, axis_dir,
                                                        sample_origin, sample_area);
                }
        }
        return total;
}

double compute_beam_score(const Scene &scene, const std::vector<Material> &mats)
{
        double score = 0.0;
        for (const auto &L : scene.lights)
        {
                if (!L.beam_spotlight)
                        continue;
                score += integrate_spotlight_area(scene, mats, L);
        }
        return score;
}

double integrate_spotlight_area_for_object(const Scene &scene,
                                          const std::vector<Material> &mats,
                                          const PointLight &L, int object_id)
{
        if (object_id < 0)
                return 0.0;
        if (!L.beam_spotlight || L.intensity <= 0.0)
                return 0.0;
        if (L.spot_radius <= 0.0)
                return 0.0;
        Basis basis = make_basis(L.direction);
        Vec3 axis_dir = basis.w;
        const int grid = 16;
        double disk_area = M_PI * L.spot_radius * L.spot_radius;
        if (disk_area <= 1e-12)
                return 0.0;
        double sample_area = disk_area / (grid * grid);

        double total = 0.0;
        for (int iy = 0; iy < grid; ++iy)
        {
                for (int ix = 0; ix < grid; ++ix)
                {
                        double su = (ix + 0.5) / static_cast<double>(grid);
                        double sv = (iy + 0.5) / static_cast<double>(grid);
                        double radius = L.spot_radius * std::sqrt(su);
                        double phi = 2.0 * M_PI * sv;
                        Vec3 offset = basis.u * (std::cos(phi) * radius) +
                                      basis.v * (std::sin(phi) * radius);
                        Vec3 sample_origin = L.position + offset + axis_dir * 1e-4;
                        total += trace_spotlight_sample(scene, mats, L, axis_dir,
                                                        sample_origin, sample_area,
                                                        object_id);
                }
        }
        return total;
}

double compute_object_beam_score(const Scene &scene,
                                const std::vector<Material> &mats, int object_id)
{
        if (object_id < 0)
                return 0.0;
        double score = 0.0;
        for (const auto &L : scene.lights)
        {
                if (!L.beam_spotlight)
                        continue;
                score += integrate_spotlight_area_for_object(scene, mats, L, object_id);
        }
        return score;
}

} // namespace

namespace
{

struct HudTextLine
{
        std::string text;
        SDL_Color color;
};

struct HudControlEntry
{
        std::string text;
        SDL_Color text_color;
        SDL_Color bar_color;
};

std::optional<int> level_suffix_from_stem(const std::string &stem)
{
        std::size_t underscore = stem.find_last_of('_');
        if (underscore == std::string::npos)
                return std::nullopt;
        std::size_t pos = underscore + 1;
        if (pos >= stem.size())
                return std::nullopt;
        int value = 0;
        bool found_digit = false;
        while (pos < stem.size())
        {
                char ch = stem[pos];
                if (!std::isdigit(static_cast<unsigned char>(ch)))
                        break;
                value = value * 10 + (ch - '0');
                found_digit = true;
                ++pos;
        }
        if (!found_digit)
                return std::nullopt;
        return value;
}

bool path_is_toml(const std::filesystem::path &path)
{
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
        });
        return ext == ".toml";
}

int parse_level_number_from_path(const std::string &scene_path)
{
        namespace fs = std::filesystem;
        fs::path path(scene_path);
        std::string stem = path.stem().string();
        auto suffix = level_suffix_from_stem(stem);
        if (!suffix)
                return 0;
        return *suffix;
}

std::string level_label_from_path(const std::string &scene_path)
{
        namespace fs = std::filesystem;
        fs::path path(scene_path);
        return path.stem().string();
}

std::vector<std::filesystem::path> collect_level_paths(const std::filesystem::path &scene_path)
{
        namespace fs = std::filesystem;
        fs::path directory = scene_path;
        if (fs::is_regular_file(directory))
                directory = directory.parent_path();
        if (directory.empty())
                directory = fs::current_path();
        std::vector<fs::path> result;
        std::error_code ec;
        for (auto &entry : fs::directory_iterator(directory, ec))
        {
                if (ec)
                        break;
                if (!entry.is_regular_file(ec))
                        continue;
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(),
                               [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                if (ext == ".toml")
                        result.push_back(fs::absolute(entry.path()));
        }
        fs::path absolute_scene = fs::absolute(scene_path);
        if (std::find(result.begin(), result.end(), absolute_scene) == result.end())
                result.push_back(absolute_scene);
        std::sort(result.begin(), result.end(), [](const fs::path &a, const fs::path &b) {
                bool a_is_toml = path_is_toml(a);
                bool b_is_toml = path_is_toml(b);
                if (a_is_toml != b_is_toml)
                        return a_is_toml;
                auto num_a = level_suffix_from_stem(a.stem().string());
                auto num_b = level_suffix_from_stem(b.stem().string());
                if (static_cast<bool>(num_a) != static_cast<bool>(num_b))
                        return static_cast<bool>(num_a);
                if (num_a && num_b && *num_a != *num_b)
                        return *num_a < *num_b;
                std::string stem_a = a.stem().string();
                std::string stem_b = b.stem().string();
                if (!stem_a.empty() && !stem_b.empty() && stem_a != stem_b)
                        return stem_a < stem_b;
                return a.string() < b.string();
        });
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
}

int level_index_for(const std::vector<std::filesystem::path> &levels,
                    const std::filesystem::path &scene_path)
{
        auto absolute_scene = std::filesystem::absolute(scene_path);
        auto it = std::find(levels.begin(), levels.end(), absolute_scene);
        if (it == levels.end())
                return 0;
        return static_cast<int>(std::distance(levels.begin(), it));
}

bool target_blinking(const Scene &scene)
{
        for (const auto &obj : scene.objects)
        {
                if (obj->shape_type() != ShapeType::BeamTarget)
                        continue;
                auto target = std::static_pointer_cast<BeamTarget>(obj);
                if (target->goal_active || target->goal_phase != 0)
                        return true;
        }
        return false;
}

} // namespace

static Vec3 trace_ray(const Scene &scene, const std::vector<Material> &mats,
                                          const Ray &r, std::mt19937 &rng,
                                          std::uniform_real_distribution<double> &dist,
                                          int depth = 0)
{
        if (depth > 10)
                return Vec3(0.0, 0.0, 0.0);
	HitRecord rec;
	if (!scene.hit(r, 1e-4, 1e9, rec))
	{
		return Vec3(0.0, 0.0, 0.0);
	}
        const Material &m = mats[rec.material_id];
        Vec3 eye = (r.dir * -1.0).normalized();
        Vec3 surface_color = surface_color_at(scene, rec, m);
        Vec3 sum = ambient_contribution(scene, surface_color);
        for (const auto &L : scene.lights)
        {
                sum += light_contribution(scene, mats, L, rec, surface_color, m, rec.p, eye);
        }
        if (m.mirror)
	{
		Vec3 refl_dir =
			r.dir - rec.normal * (2.0 * Vec3::dot(r.dir, rec.normal));
		Ray refl(rec.p + refl_dir * 1e-4, refl_dir);
                Vec3 refl_col = trace_ray(scene, mats, refl, rng, dist, depth + 1);
                double refl_ratio = REFLECTION / 100.0;
                sum = sum * (1.0 - refl_ratio) + refl_col * refl_ratio;
        }
        double alpha = compute_effective_alpha(m, rec);
        if (alpha < 1.0)
        {
                Ray next(rec.p + r.dir * 1e-4, r.dir);
                Vec3 behind = trace_ray(scene, mats, next, rng, dist, depth + 1);
                return sum * alpha + behind * (1.0 - alpha);
        }
        return sum;
}

Renderer::Renderer(Scene &s, Camera &c) : scene(s), cam(c) {}

struct Renderer::RenderState
{
        bool running = true;
        bool focused = false;
        bool edit_mode = false;
        bool rotating = false;
        int hover_obj = -1;
        int hover_mat = -1;
        int selected_obj = -1;
        int selected_mat = -1;
        double edit_dist = 0.0;
        Vec3 edit_pos;
        int spawn_key = -1;
        double fps = 0.0;
        bool scene_dirty = false;
        Uint32 last_auto_save = 0;
        double last_score = 0.0;
        int level_number = 0;
        std::string level_label;
        std::string scene_path;
        std::vector<std::filesystem::path> level_paths;
        int current_level_index = 0;
        double cumulative_score = 0.0;
        std::string player_name;
        int hud_focus_object = -1;
        double hud_focus_score = 0.0;
        bool quota_met = false;
};

void Renderer::mark_scene_dirty(RenderState &st)
{
        st.scene_dirty = true;
}

/// Initialize SDL window, renderer and texture objects.
bool Renderer::init_sdl(SDL_Window *&win, SDL_Renderer *&ren, SDL_Texture *&tex,
                                               int W, int H, int RW, int RH)
{
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
                std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
                return false;
        }
        win = SDL_CreateWindow("MiniRT", SDL_WINDOWPOS_CENTERED,
                                                   SDL_WINDOWPOS_CENTERED, W, H, 0);
        if (!win)
        {
                std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
                SDL_Quit();
                return false;
        }
        SDL_SetWindowResizable(win, SDL_FALSE);
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
        if (!ren)
        {
                std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
                SDL_DestroyWindow(win);
                SDL_Quit();
                return false;
        }
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24,
                                                         SDL_TEXTUREACCESS_STREAMING, RW, RH);
        if (!tex)
        {
                std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << "\n";
                SDL_DestroyRenderer(ren);
                SDL_DestroyWindow(win);
                SDL_Quit();
                return false;
        }
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_SetWindowGrab(win, SDL_FALSE);
        return true;
}

/// Handle SDL events, updating render state and selection.
void Renderer::process_events(RenderState &st, SDL_Window *win, SDL_Renderer *ren,
                                                        int W, int H,
                                                        std::vector<Material> &mats)
{
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
                if (e.type == SDL_QUIT)
                        st.running = false;
                else if (e.type == SDL_WINDOWEVENT &&
                                 e.window.event == SDL_WINDOWEVENT_LEAVE)
                {
                        st.focused = false;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        SDL_ShowCursor(SDL_ENABLE);
                        SDL_SetWindowGrab(win, SDL_FALSE);
                }
                else if (e.type == SDL_MOUSEBUTTONDOWN &&
                                 e.button.button == SDL_BUTTON_LEFT)
                {
                        st.focused = true;
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        SDL_ShowCursor(SDL_DISABLE);
                        SDL_SetWindowGrab(win, SDL_TRUE);
                        SDL_WarpMouseInWindow(win, W / 2, H / 2);
                        if (!st.edit_mode && st.hover_obj >= 0)
                        {
                                st.selected_obj = st.hover_obj;
                                st.selected_mat = st.hover_mat;
                                st.hover_obj = st.hover_mat = -1;
                                mats[st.selected_mat].checkered = true;
                                mats[st.selected_mat].color =
                                        mats[st.selected_mat].base_color;
                                AABB box;
                                if (scene.objects[st.selected_obj]->bounding_box(box))
                                {
                                        Vec3 center = (box.min + box.max) * 0.5;
                                        st.edit_dist =
                                                (center - cam.origin).length();
                                        Vec3 desired =
                                                cam.origin + cam.forward * st.edit_dist;
                                        Vec3 delta = desired - center;
                                        if (delta.length_squared() > 0)
                                        {
                                                Vec3 applied = scene.move_with_collision(
                                                        st.selected_obj, delta);
                                                center += applied;
                                                if (applied.length_squared() > 0)
                                                {
                                                        scene.update_beams(mats);
                                                        scene.build_bvh();
                                                        st.edit_dist =
                                                                (center - cam.origin).length();
                                                        if (g_developer_mode)
                                                                mark_scene_dirty(st);
                                                }
                                        }
                                        st.edit_pos = center;
                                }
                                st.edit_mode = true;
                                st.spawn_key = -1;
                        }
                        else if (st.edit_mode)
                        {
                                mats[st.selected_mat].checkered = false;
                                mats[st.selected_mat].color =
                                        mats[st.selected_mat].base_color;
                                st.selected_obj = st.selected_mat = -1;
                                st.edit_mode = false;
                                st.rotating = false;
                                st.spawn_key = -1;
                        }
                }
                else if (e.type == SDL_MOUSEBUTTONDOWN &&
                                 e.button.button == SDL_BUTTON_RIGHT)
                {
                        if (st.edit_mode &&
                            (g_developer_mode ||
                             (st.selected_obj >= 0 &&
                              scene.objects[st.selected_obj]->rotatable)))
                                st.rotating = true;
                }
                else if (e.type == SDL_MOUSEBUTTONUP &&
                                 e.button.button == SDL_BUTTON_RIGHT)
                {
                        st.rotating = false;
                }
                else if (g_developer_mode && st.edit_mode &&
                                 e.type == SDL_MOUSEBUTTONDOWN &&
                                 e.button.button == SDL_BUTTON_MIDDLE)
                {
                        int mid = st.selected_mat;
                        mats[mid].checkered = false;
                        mats[mid].color = mats[mid].base_color;
                        scene.objects.erase(scene.objects.begin() + st.selected_obj);
                        scene.update_beams(mats);
                        scene.build_bvh();
                        mark_scene_dirty(st);
                        st.selected_obj = st.selected_mat = -1;
                        st.edit_mode = false;
                        st.rotating = false;
                        st.spawn_key = -1;
                }
                else if (st.focused && e.type == SDL_MOUSEMOTION)
                {
                        if (st.edit_mode && st.rotating &&
                            (g_developer_mode ||
                             scene.objects[st.selected_obj]->rotatable))
                        {
                                double sens = get_mouse_sensitivity();
                                bool changed = false;
                                double yaw = -e.motion.xrel * sens;
                                if (yaw != 0.0)
                                {
                                        scene.objects[st.selected_obj]->rotate(cam.up, yaw);
                                        if (!g_developer_mode && scene.collides(st.selected_obj))
                                                scene.objects[st.selected_obj]->rotate(
                                                        cam.up, -yaw);
                                        else
                                                changed = true;
                                }
                                double pitch = -e.motion.yrel * sens;
                                if (pitch != 0.0)
                                {
                                        scene.objects[st.selected_obj]->rotate(cam.right,
                                                                                                          pitch);
                                        if (!g_developer_mode && scene.collides(st.selected_obj))
                                                scene.objects[st.selected_obj]->rotate(
                                                        cam.right, -pitch);
                                        else
                                                changed = true;
                                }
                                if (changed)
                                {
                                        scene.update_beams(mats);
                                        scene.build_bvh();
                                        if (g_developer_mode)
                                                mark_scene_dirty(st);
                                }
                        }
                        else
                        {
                                double sens = get_mouse_sensitivity();
                                cam.rotate(-e.motion.xrel * sens,
                                                   -e.motion.yrel * sens);
                        }
                }
                else if (e.type == SDL_MOUSEWHEEL)
                {
                        double step = e.wheel.y * SCROLL_STEP;
                        if (st.edit_mode)
                        {
                                if (g_developer_mode)
                                {
                                        auto obj = scene.objects[st.selected_obj];
                                        double factor = 1.0 + step;
                                        if (factor > 0.1)
                                        {
                                                switch (obj->shape_type())
                                                {
                                                case ShapeType::Sphere:
                                                {
                                                        auto sp = std::static_pointer_cast<Sphere>(obj);
                                                        sp->radius = std::max(0.1, sp->radius * factor);
                                                        break;
                                                }
                                                case ShapeType::Cube:
                                                {
                                                        auto cu = std::static_pointer_cast<Cube>(obj);
                                                        cu->half = cu->half * factor;
                                                        break;
                                                }
                                                case ShapeType::Cone:
                                                {
                                                        auto co = std::static_pointer_cast<Cone>(obj);
                                                        co->radius = std::max(0.1, co->radius * factor);
                                                        co->height = std::max(0.1, co->height * factor);
                                                        break;
                                                }
                                                case ShapeType::Cylinder:
                                                {
                                                        auto cy = std::static_pointer_cast<Cylinder>(obj);
                                                        cy->radius = std::max(0.1, cy->radius * factor);
                                                        cy->height = std::max(0.1, cy->height * factor);
                                                        break;
                                                }
                                                default:
                                                        break;
                                                }
                                                scene.update_beams(mats);
                                                scene.build_bvh();
                                                mark_scene_dirty(st);
                                        }
                                }
                                else
                                {
                                        st.edit_dist = std::clamp(st.edit_dist + step,
                                                           OBJECT_MIN_DIST,
                                                           OBJECT_MAX_DIST);
                                }
                        }
                        else if (st.focused)
                        {
                                // scene.move_camera(cam, cam.up * step, mats);
                                (void)step;
                        }
                }
                else if (g_developer_mode && st.focused && e.type == SDL_KEYDOWN &&
                                 e.key.keysym.scancode == SDL_SCANCODE_C)
                {
                        scene.update_beams(mats);
                        scene.build_bvh();
                        if (MapSaver::save(st.scene_path, scene, cam, mats))
                        {
                                std::cout << "Saved scene to: " << st.scene_path << "\n";
                                st.scene_dirty = false;
                                st.last_auto_save = SDL_GetTicks();
                        }
                        else
                        {
                                std::cerr << "Failed to save scene to: " << st.scene_path
                                          << "\n";
                                st.last_auto_save = SDL_GetTicks();
                        }
                }
                else if (g_developer_mode && st.focused && e.type == SDL_KEYDOWN &&
                                 e.key.keysym.scancode == SDL_SCANCODE_R)
                {
                        Scene backup_scene = scene;
                        Camera backup_cam = cam;
                        auto backup_mats = mats;
                        if (Parser::parse_rt_file(st.scene_path, scene, cam, W, H))
                        {
                                mats = Parser::get_materials();
                                scene.update_beams(mats);
                                scene.build_bvh();
                                st.edit_mode = false;
                                st.rotating = false;
                                st.hover_obj = -1;
                                st.hover_mat = -1;
                                st.selected_obj = -1;
                                st.selected_mat = -1;
                                st.spawn_key = -1;
                                st.edit_dist = 0.0;
                                st.edit_pos = Vec3();
                                st.scene_dirty = false;
                                st.last_auto_save = SDL_GetTicks();
                                std::cout << "Reloaded scene from: " << st.scene_path << "\n";
                        }
                        else
                        {
                                scene = std::move(backup_scene);
                                cam = backup_cam;
                                mats = std::move(backup_mats);
                                scene.update_beams(mats);
                                scene.build_bvh();
                                std::cerr << "Failed to reload scene from: " << st.scene_path
                                          << "\n";
                        }
                }
                else if (g_developer_mode && st.focused && e.type == SDL_KEYDOWN &&
                                 (e.key.keysym.scancode == SDL_SCANCODE_1 ||
                                  e.key.keysym.scancode == SDL_SCANCODE_2 ||
                                  e.key.keysym.scancode == SDL_SCANCODE_3 ||
                                  e.key.keysym.scancode == SDL_SCANCODE_4 ||
                                  e.key.keysym.scancode == SDL_SCANCODE_5))
                {
                        if (st.edit_mode)
                        {
                                if (st.spawn_key == e.key.keysym.scancode)
                                {
                                        int mid = st.selected_mat;
                                        mats[mid].checkered = false;
                                        mats[mid].color = mats[mid].base_color;
                                        scene.objects.erase(scene.objects.begin() + st.selected_obj);
                                        scene.update_beams(mats);
                                        scene.build_bvh();
                                        mark_scene_dirty(st);
                                        st.selected_obj = st.selected_mat = -1;
                                        st.edit_mode = false;
                                        st.rotating = false;
                                        st.spawn_key = -1;
                                }
                        }
                        else
                        {
                                int oid = static_cast<int>(scene.objects.size());
                                int mid = static_cast<int>(mats.size());
                                Material m;
                                m.color = m.base_color = Vec3(0.5, 0.5, 0.5);
                                m.alpha = 1.0;
                                m.mirror = true;
                                mats.push_back(m);
                                Vec3 pos = cam.origin + cam.forward * 3.0;
                                HittablePtr obj;
                                if (e.key.keysym.scancode == SDL_SCANCODE_1)
                                        obj = std::make_shared<Plane>(pos, Vec3(0, 1, 0), oid, mid);
                                else if (e.key.keysym.scancode == SDL_SCANCODE_2)
                                        obj = std::make_shared<Sphere>(pos, 1.0, oid, mid);
                                else if (e.key.keysym.scancode == SDL_SCANCODE_3)
                                        obj = std::make_shared<Cube>(pos, cam.up, 1.0, 1.0, 1.0, oid, mid);
                                else if (e.key.keysym.scancode == SDL_SCANCODE_4)
                                        obj = std::make_shared<Cone>(pos, cam.up, 1.0, 2.0, oid, mid);
                                else
                                        obj = std::make_shared<Cylinder>(pos, cam.up, 1.0, 2.0, oid, mid);
                                if (obj)
                                        obj->rotatable = obj->shape_type() != ShapeType::Plane;
                                obj->movable = true;
                                scene.objects.push_back(obj);
                                scene.update_beams(mats);
                                scene.build_bvh();
                                mark_scene_dirty(st);
                                st.selected_obj = oid;
                                st.selected_mat = mid;
                                mats[mid].checkered = true;
                                st.edit_mode = true;
                                st.rotating = false;
                                st.edit_pos = pos;
                                st.edit_dist = (pos - cam.origin).length();
                                st.spawn_key = e.key.keysym.scancode;
                        }
                }
                else if (st.focused && st.quota_met && e.type == SDL_KEYDOWN &&
                                 e.key.repeat == 0 &&
                                 (e.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                                  e.key.keysym.scancode == SDL_SCANCODE_KP_ENTER))
                {
                        st.focused = false;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        SDL_ShowCursor(SDL_ENABLE);
                        SDL_SetWindowGrab(win, SDL_FALSE);
                        SDL_WarpMouseInWindow(win, W / 2, H / 2);
                        int current_w = W;
                        int current_h = H;
                        SDL_GetWindowSize(win, &current_w, &current_h);
                        LevelFinishedStats stats;
                        int total_toml_levels = static_cast<int>(std::count_if(
                                st.level_paths.begin(), st.level_paths.end(),
                                [](const std::filesystem::path &p) { return path_is_toml(p); }));
                        stats.total_levels = total_toml_levels;
                        int completed_levels = 0;
                        if (st.current_level_index >= 0 &&
                            st.current_level_index < static_cast<int>(st.level_paths.size()))
                        {
                                for (int i = 0; i <= st.current_level_index; ++i)
                                {
                                        if (path_is_toml(st.level_paths[i]))
                                                ++completed_levels;
                                }
                        }
                        stats.completed_levels =
                                std::min(stats.total_levels, completed_levels);
                        stats.current_score = st.last_score;
                        stats.required_score = scene.minimal_score;
                        stats.total_score = st.cumulative_score + st.last_score;
                        bool has_next_level = false;
                        if (st.current_level_index >= 0 &&
                            st.current_level_index < static_cast<int>(st.level_paths.size()))
                        {
                                for (int i = st.current_level_index + 1;
                                     i < static_cast<int>(st.level_paths.size()); ++i)
                                {
                                        if (path_is_toml(st.level_paths[i]))
                                        {
                                                has_next_level = true;
                                                break;
                                        }
                                }
                        }
                        stats.has_next_level = has_next_level;
                        ButtonAction action = LevelFinishedMenu::show(
                                win, ren, current_w, current_h, stats, st.player_name, true);
                        if (action == ButtonAction::Quit)
                        {
                                st.running = false;
                        }
                        else if (action == ButtonAction::NextLevel)
                        {
                                if (stats.has_next_level)
                                {
                                        auto next_it = std::find_if(
                                                st.level_paths.begin() + st.current_level_index + 1,
                                                st.level_paths.end(),
                                                [](const std::filesystem::path &p) {
                                                        return path_is_toml(p);
                                                });
                                        if (next_it != st.level_paths.end())
                                        {
                                                const auto &next_path = *next_it;
                                                Scene backup_scene = scene;
                                                Camera backup_cam = cam;
                                                auto backup_mats = mats;
                                                if (Parser::parse_rt_file(next_path.string(), scene, cam, W, H))
                                                {
                                                        mats = Parser::get_materials();
                                                        scene.update_beams(mats);
                                                        scene.build_bvh();
                                                        st.cumulative_score += st.last_score;
                                                        st.current_level_index = static_cast<int>(
                                                                std::distance(st.level_paths.begin(), next_it));
                                                        st.scene_path = next_path.string();
                                                        st.level_number =
                                                                parse_level_number_from_path(st.scene_path);
                                                        st.level_label =
                                                                level_label_from_path(st.scene_path);
                                                        st.scene_dirty = false;
                                                        st.last_auto_save = SDL_GetTicks();
                                                        st.edit_mode = false;
                                                        st.rotating = false;
                                                        st.hover_obj = -1;
                                                        st.hover_mat = -1;
                                                        st.selected_obj = -1;
                                                        st.selected_mat = -1;
                                                        st.spawn_key = -1;
                                                        st.edit_dist = 0.0;
                                                        st.edit_pos = Vec3();
                                                        st.quota_met = false;
                                                        st.last_score = 0.0;
                                                }
                                                else
                                                {
                                                        scene = std::move(backup_scene);
                                                        cam = backup_cam;
                                                        mats = std::move(backup_mats);
                                                        scene.update_beams(mats);
                                                        scene.build_bvh();
                                                        std::cerr << "Failed to load next level: "
                                                                  << next_path << "\n";
                                                }
                                        }
                                }
                                st.focused = true;
                                SDL_SetRelativeMouseMode(SDL_TRUE);
                                SDL_ShowCursor(SDL_DISABLE);
                                SDL_SetWindowGrab(win, SDL_TRUE);
                                SDL_WarpMouseInWindow(win, W / 2, H / 2);
                        }
                        else
                        {
                                st.focused = true;
                                SDL_SetRelativeMouseMode(SDL_TRUE);
                                SDL_ShowCursor(SDL_DISABLE);
                                SDL_SetWindowGrab(win, SDL_TRUE);
                                SDL_WarpMouseInWindow(win, W / 2, H / 2);
                        }
                }
                else if (st.focused && e.type == SDL_KEYDOWN &&
                                 e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                        st.focused = false;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        SDL_ShowCursor(SDL_ENABLE);
                        SDL_SetWindowGrab(win, SDL_FALSE);
                        SDL_WarpMouseInWindow(win, W / 2, H / 2);
                        int current_w = W;
                        int current_h = H;
                        SDL_GetWindowSize(win, &current_w, &current_h);
                        bool resume = PauseMenu::show(win, ren, current_w, current_h);
                        if (resume)
                        {
                                st.focused = true;
                                SDL_SetRelativeMouseMode(SDL_TRUE);
                                SDL_ShowCursor(SDL_DISABLE);
                                SDL_SetWindowGrab(win, SDL_TRUE);
                                SDL_WarpMouseInWindow(win, W / 2, H / 2);
                        }
                        else
                        {
                                st.running = false;
                        }
                }
        }
}

/// Handle continuous keyboard input for camera or object control.
void Renderer::handle_keyboard(RenderState &st, double dt,
                                                          std::vector<Material> &mats)
{
        const Uint8 *state = SDL_GetKeyboardState(nullptr);
        Vec3 forward_xz = cam.forward;
        forward_xz.y = 0.0;
        if (forward_xz.length_squared() > 0.0)
                forward_xz = forward_xz.normalized();
        Vec3 right_xz = cam.right;

        if (st.edit_mode)
        {
                double cam_speed = CAMERA_MOVE_SPEED * dt;
                if (state[SDL_SCANCODE_W])
                        scene.move_camera(cam, forward_xz * cam_speed, mats);
                if (state[SDL_SCANCODE_S])
                        scene.move_camera(cam, forward_xz * -cam_speed, mats);
                if (state[SDL_SCANCODE_A])
                        scene.move_camera(cam, right_xz * -cam_speed, mats);
                if (state[SDL_SCANCODE_D])
                        scene.move_camera(cam, right_xz * cam_speed, mats);
                if (state[SDL_SCANCODE_SPACE])
                        scene.move_camera(cam, Vec3(0, 1, 0) * cam_speed, mats);
                if (state[SDL_SCANCODE_LCTRL])
                        scene.move_camera(cam, Vec3(0, -1, 0) * cam_speed, mats);

                double rot_speed = OBJECT_ROTATE_SPEED * dt;
                bool changed = false;
                bool can_rotate = g_developer_mode ||
                                  scene.objects[st.selected_obj]->rotatable;
                if (can_rotate && state[SDL_SCANCODE_Q])
                {
                        scene.objects[st.selected_obj]->rotate(cam.forward, -rot_speed);
                        if (!g_developer_mode && scene.collides(st.selected_obj))
                                scene.objects[st.selected_obj]->rotate(cam.forward,
                                                                                                      rot_speed);
                        else
                                changed = true;
                }
                if (can_rotate && state[SDL_SCANCODE_E])
                {
                        scene.objects[st.selected_obj]->rotate(cam.forward, rot_speed);
                        if (!g_developer_mode && scene.collides(st.selected_obj))
                                scene.objects[st.selected_obj]->rotate(cam.forward,
                                                                                                      -rot_speed);
                        else
                                changed = true;
                }
                if (changed)
                {
                        scene.update_beams(mats);
                        scene.build_bvh();
                        if (g_developer_mode)
                                mark_scene_dirty(st);
                }
        }
        else if (st.focused)
        {
                double speed = CAMERA_MOVE_SPEED * dt;
                if (state[SDL_SCANCODE_W])
                        scene.move_camera(cam, forward_xz * speed, mats);
                if (state[SDL_SCANCODE_S])
                        scene.move_camera(cam, forward_xz * -speed, mats);
                if (state[SDL_SCANCODE_A])
                        scene.move_camera(cam, right_xz * -speed, mats);
                if (state[SDL_SCANCODE_D])
                        scene.move_camera(cam, right_xz * speed, mats);
                if (state[SDL_SCANCODE_SPACE])
                        scene.move_camera(cam, Vec3(0, 1, 0) * speed, mats);
                if (state[SDL_SCANCODE_LCTRL])
                        scene.move_camera(cam, Vec3(0, -1, 0) * speed, mats);
        }
}

/// Update object selection or hover based on current state.
void Renderer::update_selection(RenderState &st,
                                                          std::vector<Material> &mats)
{
        auto refresh_hud_focus = [&](bool allow_hover) {
                st.hud_focus_object = -1;
                st.hud_focus_score = 0.0;
                Ray center_ray = cam.ray_through(0.5, 0.5);
                HitRecord hrec;
                if (!scene.hit(center_ray, 1e-4, 1e9, hrec))
                {
                        if (allow_hover && st.hover_mat >= 0)
                        {
                                mats[st.hover_mat].color =
                                        mats[st.hover_mat].base_color;
                                st.hover_obj = st.hover_mat = -1;
                        }
                        return;
                }

                auto &hit_obj = scene.objects[hrec.object_id];
                ShapeType shape = hit_obj->shape_type();
                if (shape != ShapeType::Plane && shape != ShapeType::BeamTarget &&
                    shape != ShapeType::Beam)
                {
                        st.hud_focus_object = hrec.object_id;
                        st.hud_focus_score =
                                compute_object_beam_score(scene, mats, hrec.object_id);
                }

                if (!allow_hover)
                        return;
                bool selectable = !hit_obj->is_beam() &&
                                  (g_developer_mode || hit_obj->movable ||
                                   hit_obj->rotatable);
                if (selectable)
                {
                        if (st.hover_mat != hrec.material_id)
                        {
                                if (st.hover_mat >= 0)
                                        mats[st.hover_mat].color =
                                                mats[st.hover_mat].base_color;
                                st.hover_obj = hrec.object_id;
                                st.hover_mat = hrec.material_id;
                        }
                        bool blink = ((SDL_GetTicks() / 250) % 2) == 0;
                        Vec3 base_color = mats[st.hover_mat].base_color;
                        Vec3 alt_color = highlight_alternate_color(base_color);
                        mats[st.hover_mat].color = blink ? alt_color : base_color;
                }
                else if (st.hover_mat >= 0)
                {
                        mats[st.hover_mat].color = mats[st.hover_mat].base_color;
                        st.hover_obj = st.hover_mat = -1;
                }
        };

        if (st.edit_mode)
        {
                if (st.hover_mat >= 0 && st.hover_mat != st.selected_mat)
                        mats[st.hover_mat].color =
                                mats[st.hover_mat].base_color;
                st.hover_obj = st.hover_mat = -1;

                Vec3 desired = cam.origin + cam.forward * st.edit_dist;
                Vec3 delta = desired - st.edit_pos;
                if (delta.length_squared() > 0)
                {
                        Vec3 applied = scene.move_with_collision(st.selected_obj, delta);
                        st.edit_pos += applied;
                        if (applied.length_squared() > 0)
                        {
                                scene.update_beams(mats);
                                scene.build_bvh();
                                if (g_developer_mode)
                                        mark_scene_dirty(st);
                        }
                }

                Vec3 cam_target = st.edit_pos - cam.forward * st.edit_dist;
                Vec3 cam_delta = cam_target - cam.origin;
                if (cam_delta.length_squared() > 0)
                        scene.move_camera(cam, cam_delta, mats);
                st.edit_dist = (st.edit_pos - cam.origin).length();

                refresh_hud_focus(false);
        }
        else
        {
                refresh_hud_focus(true);
        }
}

int Renderer::render_hud(const RenderState &st, SDL_Renderer *ren, int W, int H)
{
        const int hud_scale = 2;
        const int hud_padding = 12;

        std::vector<HudTextLine> left_lines;
        std::string level_line;
        if (st.level_number > 0)
                level_line = "LEVEL " + std::to_string(st.level_number);
        else if (!st.level_label.empty())
                level_line = "LEVEL " + st.level_label;
        else
                level_line = "LEVEL ?";
        left_lines.push_back({level_line, SDL_Color{255, 255, 255, 255}});

        if (scene.target_required)
        {
                bool target_hit = target_blinking(scene);
                SDL_Color target_color =
                        target_hit ? SDL_Color{96, 255, 128, 255}
                                   : SDL_Color{255, 96, 96, 255};
                left_lines.push_back({std::string("TARGET: ") +
                                              (target_hit ? "HIT" : "NOT HIT"),
                                      target_color});
        }

        char score_buf[64];
        if (scene.minimal_score > 0.0)
        {
                std::snprintf(score_buf, sizeof(score_buf), "SCORE: %.2f/%.2f",
                              st.last_score, scene.minimal_score);
        }
        else
        {
                std::snprintf(score_buf, sizeof(score_buf), "SCORE: %.2f", st.last_score);
        }
        left_lines.push_back({score_buf, SDL_Color{255, 255, 255, 255}});

        std::vector<HudTextLine> center_lines;
        if (st.quota_met)
        {
                center_lines.push_back(
                        {"LEVEL FINISHED", SDL_Color{255, 255, 255, 255}});
                center_lines.push_back(
                        {"ENTER TO CONTINUE", SDL_Color{255, 255, 255, 255}});
        }

        std::vector<HudTextLine> right_lines;
        std::shared_ptr<Hittable> focus_obj;
        if (st.hud_focus_object >= 0 &&
            st.hud_focus_object < static_cast<int>(scene.objects.size()))
                focus_obj = scene.objects[st.hud_focus_object];
        std::optional<std::string> focus_hint_label;
        auto shape_label_for = [](ShapeType shape) {
                switch (shape)
                {
                case ShapeType::Sphere:
                        return std::string("SPHERE");
                case ShapeType::Cube:
                        return std::string("CUBE");
                case ShapeType::Cylinder:
                        return std::string("CYLINDER");
                case ShapeType::Cone:
                        return std::string("CONE");
                case ShapeType::Plane:
                        return std::string("PLANE");
                case ShapeType::BeamTarget:
                        return std::string("TARGET");
                case ShapeType::Beam:
                        return std::string("LASER");
                default:
                        return std::string("OBJECT");
                }
        };
        if (focus_obj)
        {
                if (auto source = std::dynamic_pointer_cast<BeamSource>(focus_obj))
                {
                        SDL_Color beam_color{200, 200, 200, 255};
                        std::string beam_status = "BEAM - NONE";
                        double beam_length = 0.0;
                        double beam_power = 0.0;
                        if (source->movable)
                        {
                                beam_status = "BEAM - MOVABLE";
                                beam_color = SDL_Color{96, 255, 128, 255};
                        }
                        else if (source->rotatable)
                        {
                                beam_status = "BEAM - ROTATABLE";
                                beam_color = SDL_Color{255, 220, 96, 255};
                        }
                        else
                        {
                                beam_status = "BEAM - STATIONARY";
                                beam_color = SDL_Color{255, 96, 96, 255};
                        }
                        if (source->beam)
                        {
                                beam_length = source->beam->total_length;
                                beam_power = source->beam->light_intensity;
                        }
                        if (beam_power <= 0.0 && source->light)
                                beam_power = source->light->intensity;
                        if (beam_length <= 0.0)
                        {
                                for (const auto &L : scene.lights)
                                {
                                        if (L.attached_id == source->object_id)
                                        {
                                                if (L.range > 0.0)
                                                        beam_length = L.range;
                                                if (beam_power <= 0.0)
                                                        beam_power = L.intensity;
                                                break;
                                        }
                                }
                        }
                        right_lines.push_back({beam_status, beam_color});

                        char length_buf[64];
                        if (beam_length > 0.0)
                                std::snprintf(length_buf, sizeof(length_buf),
                                              "LENGTH: %.1f m", beam_length);
                        else
                                std::snprintf(length_buf, sizeof(length_buf), "LENGTH: --");
                        right_lines.push_back({length_buf, SDL_Color{255, 255, 255, 255}});

                        char power_buf[64];
                        if (beam_power > 0.0)
                                std::snprintf(power_buf, sizeof(power_buf), "POWER: %.2f",
                                              beam_power);
                        else
                                std::snprintf(power_buf, sizeof(power_buf), "POWER: --");
                        right_lines.push_back({power_buf, SDL_Color{255, 255, 255, 255}});

                        focus_hint_label = std::string("BEAM SOURCE");
                }
                else
                {
                        auto shape = focus_obj->shape_type();
                        auto shape_label = shape_label_for(shape);

                        SDL_Color status_color{200, 200, 200, 255};
                        std::string status_text = shape_label + " - STATIONARY";
                        if (focus_obj->movable)
                        {
                                status_text = shape_label + " - MOVABLE";
                                status_color = SDL_Color{96, 255, 128, 255};
                        }
                        else if (focus_obj->rotatable)
                        {
                                status_text = shape_label + " - ROTATABLE";
                                status_color = SDL_Color{255, 220, 96, 255};
                        }
                        else
                        {
                                status_color = SDL_Color{255, 96, 96, 255};
                        }
                        right_lines.push_back({status_text, status_color});

                        char score_line[64];
                        std::snprintf(score_line, sizeof(score_line), "OBJECT SCORE: %.2f",
                                      st.hud_focus_score);
                        right_lines.push_back({score_line, SDL_Color{255, 255, 255, 255}});
                        if (shape != ShapeType::Plane && shape != ShapeType::BeamTarget &&
                            shape != ShapeType::Beam)
                                focus_hint_label = shape_label;
                }
        }

        constexpr size_t kControlSections = 5;
        std::array<std::optional<HudControlEntry>, kControlSections> control_sections;
        control_sections.fill(std::nullopt);
        const SDL_Color bar_neutral{72, 72, 72, 220};
        const SDL_Color command_divider{80, 80, 80, 220};
        auto set_control = [&](size_t index, const std::string &label, SDL_Color text_color) {
                if (index < control_sections.size())
                        control_sections[index] = HudControlEntry{label, text_color, bar_neutral};
        };
        SDL_Color neutral{255, 255, 255, 255};
        SDL_Color accent{96, 255, 128, 255};
        SDL_Color warning{255, 220, 96, 255};
        SDL_Color danger{255, 96, 96, 255};

        const size_t slot_move = 0;
        const size_t slot_rotate = 1;
        const size_t slot_primary = 2;
        const size_t slot_secondary = 3;
        const size_t slot_pause = 4;

        set_control(slot_pause, "PAUSE\nESC", neutral);

        if (!st.focused)
        {
                set_control(slot_move, "FOCUS LOST\nCLICK WINDOW", warning);
                set_control(slot_rotate, "RESUME CONTROL\nCLICK", neutral);
        }
        else if (st.edit_mode)
        {
                std::shared_ptr<Hittable> selected_obj;
                if (st.selected_obj >= 0 &&
                    st.selected_obj < static_cast<int>(scene.objects.size()))
                        selected_obj = scene.objects[st.selected_obj];

                if (selected_obj)
                {
                        bool can_move = !selected_obj->is_beam() &&
                                        (g_developer_mode || selected_obj->movable);
                        bool can_rotate = g_developer_mode || selected_obj->rotatable;

                        if (can_move)
                                set_control(slot_move, "MOVE\nWSAD\nCTRL/SPACE", neutral);
                        if (can_rotate)
                                set_control(slot_rotate, "ROTATE\nHOLD RBM\nQ/E", neutral);

                        set_control(slot_primary, "PLACE\nLBM", accent);
                        if (g_developer_mode)
                        {
                                std::string dev_text =
                                        "DEV TOOLS\nRESIZE - SCROLL\nDELETE - MMB";
                                set_control(slot_secondary, dev_text, danger);
                        }
                }
                else
                {
                        set_control(slot_move, "MOVE\nWSAD\nCTRL/SPACE", neutral);
                        set_control(slot_primary, "PLACE\nLBM", accent);
                }
        }
        else
        {
                set_control(slot_move, "MOVE\nWSAD\nCTRL/SPACE", neutral);

                bool show_grab = false;
                if (focus_obj)
                {
                        bool grabbable = !focus_obj->is_beam() &&
                                         (g_developer_mode || focus_obj->movable ||
                                          focus_obj->rotatable);
                        show_grab = grabbable;
                }

                if (show_grab)
                {
                        set_control(slot_secondary, "GRAB\nLBM", accent);
                }
        }

        auto split_lines = [](const std::string &text) {
                std::vector<std::string> lines;
                size_t start = 0;
                while (start <= text.size())
                {
                        size_t pos = text.find('\n', start);
                        if (pos == std::string::npos)
                        {
                                lines.emplace_back(text.substr(start));
                                break;
                        }
                        lines.emplace_back(text.substr(start, pos - start));
                        start = pos + 1;
                }
                if (lines.empty())
                        lines.emplace_back(std::string());
                while (lines.size() > 1 && lines.back().empty())
                        lines.pop_back();
                if (lines.size() == 1 && lines.front().empty())
                        lines.clear();
                return lines;
        };

        std::array<std::vector<std::string>, kControlSections> section_lines{};
        size_t max_control_lines = 1;
        for (size_t i = 0; i < control_sections.size(); ++i)
        {
                if (!control_sections[i])
                        continue;
                section_lines[i] = split_lines(control_sections[i]->text);
                if (!section_lines[i].empty())
                        max_control_lines =
                                std::max(max_control_lines, section_lines[i].size());
        }
        std::vector<size_t> active_sections;
        active_sections.reserve(control_sections.size());
        for (size_t i = 0; i < control_sections.size(); ++i)
        {
                if (!section_lines[i].empty())
                        active_sections.push_back(i);
        }
        if (active_sections.empty())
        {
                for (size_t i = 0; i < control_sections.size(); ++i)
                {
                        if (control_sections[i])
                        {
							active_sections.push_back(i);
							break;
                        }
                }
                if (active_sections.empty())
                        active_sections.push_back(0);
        }

        const int hud_line_height = 7 * hud_scale + 4;

        size_t top_count = std::max(left_lines.size(), right_lines.size());
        top_count = std::max(top_count, center_lines.size());
        if (top_count == 0)
                top_count = 1;
        int top_bar_height = static_cast<int>(top_count) * hud_line_height + 2 * hud_padding;
        top_bar_height = std::max(top_bar_height, hud_line_height + 2 * hud_padding);

        int center_rect_left = 0;
        int center_rect_width = 0;
        if (!center_lines.empty())
        {
                int max_center_text_width = 0;
                for (const auto &line : center_lines)
                {
                        max_center_text_width = std::max(
                                max_center_text_width,
                                CustomCharacter::text_width(line.text, hud_scale));
                }
                int padded_width = max_center_text_width + hud_padding * 2;
                center_rect_width = std::min(W, std::max(padded_width, max_center_text_width));
                center_rect_left = (W - center_rect_width) / 2;
        }

        int bottom_bar_height = static_cast<int>(max_control_lines) * hud_line_height +
                                2 * hud_padding;

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
        SDL_Rect top_bar{0, 0, W, top_bar_height};
        SDL_RenderFillRect(ren, &top_bar);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 150);
        SDL_Rect bottom_bar{0, H - bottom_bar_height, W, bottom_bar_height};
        SDL_RenderFillRect(ren, &bottom_bar);

        if (!center_lines.empty() && center_rect_width > 0)
        {
                Uint32 ticks = SDL_GetTicks();
                bool highlight_on = ((ticks / 350) % 2) == 0;
                if (highlight_on)
                {
                        SDL_SetRenderDrawColor(ren, 255, 240, 128, 180);
                        SDL_Rect highlight_rect{center_rect_left, 0, center_rect_width,
                                                top_bar_height};
                        SDL_RenderFillRect(ren, &highlight_rect);
                }
        }

        if (top_bar_height > 2 * hud_padding)
        {
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 96);
                if (!center_lines.empty() && center_rect_width > 0)
                {
                        int left_sep = center_rect_left;
                        int right_sep = center_rect_left + center_rect_width;
                        SDL_RenderDrawLine(ren, left_sep, hud_padding, left_sep,
                                           top_bar_height - hud_padding);
                        SDL_RenderDrawLine(ren, right_sep, hud_padding, right_sep,
                                           top_bar_height - hud_padding);
                }
                else
                {
                        SDL_RenderDrawLine(ren, W / 2, hud_padding, W / 2,
                                           top_bar_height - hud_padding);
                }
        }

        int left_y = hud_padding;
        for (const auto &line : left_lines)
        {
                CustomCharacter::draw_text(ren, line.text, hud_padding, left_y, line.color,
                                            hud_scale);
                left_y += hud_line_height;
        }

        if (!center_lines.empty())
        {
                int center_y = hud_padding;
                for (const auto &line : center_lines)
                {
                        int width = CustomCharacter::text_width(line.text, hud_scale);
                        int text_x = W / 2 - width / 2;
                        CustomCharacter::draw_text(ren, line.text, text_x, center_y, line.color,
                                                    hud_scale);
                        center_y += hud_line_height;
                }
        }

        int right_y = hud_padding;
        for (const auto &line : right_lines)
        {
                int width = CustomCharacter::text_width(line.text, hud_scale);
                int text_x = std::max(hud_padding, W - hud_padding - width);
                CustomCharacter::draw_text(ren, line.text, text_x, right_y, line.color,
                                            hud_scale);
                right_y += hud_line_height;
        }

        int controls_top = H - bottom_bar_height + hud_padding;
        size_t section_count = active_sections.size();
        double section_span = static_cast<double>(W) /
                              static_cast<double>(std::max<size_t>(section_count, 1));
        int bar_vertical_margin = std::max(2, hud_padding / 2);
        int bar_horizontal_margin = std::max(2, hud_padding / 2);
        int bar_top = H - bottom_bar_height + bar_vertical_margin;
        int bar_height = std::max(0, bottom_bar_height - 2 * bar_vertical_margin);
        SDL_Color separator_color{255, 255, 255, 96};
        for (size_t pos = 0; pos < active_sections.size(); ++pos)
        {
                size_t i = active_sections[pos];
                int start_x = static_cast<int>(std::round(pos * section_span));
                int end_x = static_cast<int>(std::round((pos + 1) * section_span));
                int available = std::max(1, end_x - start_x);
                int bar_left = start_x + bar_horizontal_margin;
                int bar_width = std::max(0, available - 2 * bar_horizontal_margin);
                SDL_Rect bar_rect{bar_left, bar_top, bar_width, bar_height};
                auto compute_text_x = [&](int width) {
                        int min_x = start_x + hud_padding;
                        int max_x = end_x - hud_padding - width;
                        if (max_x < min_x)
                                max_x = min_x;
                        int centered = start_x + (available - width) / 2;
                        return std::clamp(centered, min_x, max_x);
                };
                if (control_sections[i] && !section_lines[i].empty())
                {
                        const auto &entry = *control_sections[i];
                        if (bar_rect.w > 0 && bar_rect.h > 0)
                        {
                                SDL_SetRenderDrawColor(ren, entry.bar_color.r,
                                                       entry.bar_color.g, entry.bar_color.b,
                                                       entry.bar_color.a);
                                SDL_RenderFillRect(ren, &bar_rect);
                        }

                        const auto &lines = section_lines[i];
                        int header_y = controls_top;
                        if (!lines.front().empty())
                        {
                                int header_width =
                                        CustomCharacter::text_width(lines.front(), hud_scale);
                                int header_x = compute_text_x(header_width);
                                CustomCharacter::draw_text(ren, lines.front(), header_x, header_y,
                                                            entry.text_color, hud_scale);
                        }

                        if (lines.size() > 1)
                        {
                                int full_left = start_x + hud_padding;
                                int full_right = end_x - hud_padding;
                                if (full_right > full_left)
                                {
                                        int divider_gap = 2;
                                        int divider_height = 1;
                                        int total_span = full_right - full_left;
                                        int divider_width = std::max(1, total_span / 2);
                                        int divider_left = full_left + (total_span - divider_width) / 2;
                                        int divider_top = header_y + hud_line_height -
                                                          divider_height - divider_gap;
                                        divider_top = std::clamp(divider_top, bar_top,
                                                                 bar_top + bar_height -
                                                                         divider_height);
                                        SDL_Rect divider_rect{divider_left, divider_top,
                                                              divider_width, divider_height};
                                        SDL_SetRenderDrawColor(ren, command_divider.r,
                                                               command_divider.g,
                                                               command_divider.b,
                                                               command_divider.a);
                                        SDL_RenderFillRect(ren, &divider_rect);

                                        int divider_bottom = divider_top + divider_height;
                                        int controls_area_top = divider_bottom + divider_gap;
                                        int controls_area_bottom =
                                                std::min(H - hud_padding, bar_top + bar_height);
                                        if (controls_area_bottom < controls_area_top)
                                                controls_area_bottom = controls_area_top;
                                        size_t control_line_count = lines.size() - 1;
                                        int control_block_height =
                                                static_cast<int>(control_line_count) *
                                                hud_line_height;
                                        int available_height = controls_area_bottom -
                                                              controls_area_top;
                                        int control_text_y = controls_area_top;
                                        if (available_height > control_block_height)
                                                control_text_y +=
                                                        (available_height - control_block_height) / 2;

                                        for (size_t line_idx = 1; line_idx < lines.size();
                                             ++line_idx)
                                        {
                                                const auto &line = lines[line_idx];
                                                if (!line.empty())
                                                {
                                                        int line_width = CustomCharacter::text_width(
                                                                line, hud_scale);
                                                        int text_x = compute_text_x(line_width);
                                                        CustomCharacter::draw_text(ren, line, text_x,
                                                                                  control_text_y,
                                                                                  entry.text_color,
                                                                                  hud_scale);
                                                }
                                                control_text_y += hud_line_height;
                                        }
                                }
                        }
                }
                if (pos + 1 < active_sections.size())
                {
                        SDL_SetRenderDrawColor(ren, separator_color.r, separator_color.g,
                                               separator_color.b, separator_color.a);
                        SDL_RenderDrawLine(ren, end_x, H - bottom_bar_height, end_x, H);
                }
        }

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        return top_bar_height;
}

/// Render the current frame and display it to the window.
void Renderer::render_frame(RenderState &st, SDL_Renderer *ren, SDL_Texture *tex,
                                                       std::vector<Vec3> &framebuffer,
                                                       std::vector<unsigned char> &pixels,
                                                       int RW, int RH, int W, int H, int T,
                                                       std::vector<Material> &mats)
{
        std::atomic<int> next_row{0};
        auto worker = [&](int index)
        {
                std::mt19937 rng(std::random_device{}());
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                for (;;)
                {
                        int y = next_row.fetch_add(1);
                        if (y >= RH)
                                break;
                        for (int x = 0; x < RW; ++x)
                        {
                                double u = (x + 0.5) / static_cast<double>(RW);
                                double v = (y + 0.5) / static_cast<double>(RH);
                                Ray r = cam.ray_through(u, v);
                                Vec3 col = trace_ray(scene, mats, r, rng, dist, 0);
                                framebuffer[y * RW + x] = col;
                        }
                }
        };

        std::vector<std::thread> pool;
        pool.reserve(T);
        for (int i = 0; i < T; ++i)
                pool.emplace_back(worker, i);
        for (auto &th : pool)
                th.join();

        st.last_score = compute_beam_score(scene, mats);

        bool quota_defined = (scene.minimal_score > 0.0) || scene.target_required;
        bool score_met = (scene.minimal_score <= 0.0) ||
                         (st.last_score + kQuotaScoreEpsilon >= scene.minimal_score);
        bool target_met = !scene.target_required || target_blinking(scene);
        st.quota_met = quota_defined && score_met && target_met;

        for (int y = 0; y < RH; ++y)
        {
                for (int x = 0; x < RW; ++x)
                {
                        Vec3 c = framebuffer[y * RW + x];
                        c.x = std::clamp(c.x, 0.0, 1.0);
                        c.y = std::clamp(c.y, 0.0, 1.0);
                        c.z = std::clamp(c.z, 0.0, 1.0);
                        pixels[(y * RW + x) * 3 + 0] =
                                static_cast<unsigned char>(std::lround(c.x * 255.0));
                        pixels[(y * RW + x) * 3 + 1] =
                                static_cast<unsigned char>(std::lround(c.y * 255.0));
                        pixels[(y * RW + x) * 3 + 2] =
                                static_cast<unsigned char>(std::lround(c.z * 255.0));
                }
        }

        SDL_UpdateTexture(tex, nullptr, pixels.data(), RW * 3);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        int top_bar_height = render_hud(st, ren, W, H);
        int legend_base_y = top_bar_height + 5;
        if (st.edit_mode && g_developer_mode)
        {
                auto project = [&](const Vec3 &p, int &sx, int &sy) -> bool
                {
                        Vec3 rel = p - cam.origin;
                        double z = Vec3::dot(rel, cam.forward);
                        if (z <= 0.0)
                                return false;
                        double x = Vec3::dot(rel, cam.right);
                        double y = Vec3::dot(rel, cam.up);
                        double fov_rad = cam.fov_deg * M_PI / 180.0;
                        double half_h = std::tan(fov_rad * 0.5);
                        double half_w = cam.aspect * half_h;
                        double u = (x / z / half_w + 1.0) * 0.5;
                        double v = (1.0 - y / z / half_h) * 0.5;
                        sx = static_cast<int>(u * W);
                        sy = static_cast<int>(v * H);
                        return true;
                };
                const int edges[12][2] = {{0, 1},  {0, 2},  {0, 4},  {1, 3},
                                                                           {1, 5},  {2, 3},  {2, 6},  {3, 7},
                                                                           {4, 5},  {4, 6},  {5, 7},  {6, 7}};
                for (size_t i = 0; i < scene.objects.size(); ++i)
                {
                        auto &obj = scene.objects[i];
                        if (obj->is_beam() || obj->is_plane())
                                continue;
                        AABB box;
                        if (!obj->bounding_box(box))
                                continue;
                        Vec3 corners[8] = {Vec3(box.min.x, box.min.y, box.min.z),
                                                           Vec3(box.max.x, box.min.y, box.min.z),
                                                           Vec3(box.min.x, box.max.y, box.min.z),
                                                           Vec3(box.max.x, box.max.y, box.min.z),
                                                           Vec3(box.min.x, box.min.y, box.max.z),
                                                           Vec3(box.max.x, box.min.y, box.max.z),
                                                           Vec3(box.min.x, box.max.y, box.max.z),
                                                           Vec3(box.max.x, box.max.y, box.max.z)};
                        SDL_Point pts[8];
                        bool ok = true;
                        for (int j = 0; j < 8; ++j)
                        {
                                int sx, sy;
                                if (!project(corners[j], sx, sy))
                                {
                                        ok = false;
                                        break;
                                }
                                pts[j].x = sx;
                                pts[j].y = sy;
                        }
                        if (!ok)
                                continue;
                        if (static_cast<int>(i) == st.selected_obj)
                                SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
                        else
                                SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
                        for (const auto &e : edges)
                                SDL_RenderDrawLine(ren, pts[e[0]].x, pts[e[0]].y,
                                                                                  pts[e[1]].x, pts[e[1]].y);
                }
        }
        // Draw crosshair at the center of the screen
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        int cx = W / 2;
        int cy = H / 2;
        const int ch_size = 10;
        SDL_RenderDrawLine(ren, cx - ch_size, cy, cx + ch_size, cy);
        SDL_RenderDrawLine(ren, cx, cy - ch_size, cx, cy + ch_size);
        if (g_developer_mode)
        {
                SDL_Color red{255, 0, 0, 255};
                int scale = 2;
                const char *legend[] = {"1-PLANE",  "2-SPHERE",   "3-CUBE",
                                         "4-CONE",  "5-CYLINDER", "SCROLL-SIZE",
                                         "MCLICK-DEL"};
                for (int i = 0; i < 7; ++i)
                        CustomCharacter::draw_text(ren, legend[i], 5,
                                                    legend_base_y + i * (7 * scale + 2), red,
                                                    scale);
                std::string text = "DEVELOPER MODE";
                int tw = CustomCharacter::text_width(text, scale);
                CustomCharacter::draw_text(ren, text, W - tw - 5, 5, red, scale);
                double fps_value = std::min(st.fps, 9999.9);
                char fps_buf[32];
                std::snprintf(fps_buf, sizeof(fps_buf), "FPS: %.1f", fps_value);
                std::string fps_text(fps_buf);
                int fps_w = CustomCharacter::text_width(fps_text, scale);
                int fps_h = 7 * scale;
                int fps_x = std::max(0, W - fps_w - 5);
                int fps_y = std::max(0, H - fps_h - 5);
                CustomCharacter::draw_text(ren, fps_text, fps_x, fps_y, red, scale);
        }
        SDL_RenderPresent(ren);
}

void Renderer::render_ppm(const std::string &path,
						  const std::vector<Material> &mats,
						  const RenderSettings &rset)
{
	const float scale = std::max(1.0f, rset.downscale);
	const int W = std::max(1, static_cast<int>(rset.width / scale));
	const int H = std::max(1, static_cast<int>(rset.height / scale));
	const int T = (rset.threads > 0)
					  ? rset.threads
					  : (std::thread::hardware_concurrency()
							 ? (int)std::thread::hardware_concurrency()
							 : 8);

	std::vector<Vec3> framebuffer(W * H);
	std::atomic<int> next_row{0};

	auto worker = [&]()
	{
		std::mt19937 rng(std::random_device{}());
		std::uniform_real_distribution<double> dist(0.0, 1.0);
		for (;;)
		{
			int y = next_row.fetch_add(1);
			if (y >= H)
				break;
			for (int x = 0; x < W; ++x)
			{
				double u = (x + 0.5) / W;
				double v = (y + 0.5) / H;
				Ray r = cam.ray_through(u, v);
				Vec3 col = trace_ray(scene, mats, r, rng, dist);
				framebuffer[y * W + x] = col;
			}
		}
	};

	std::vector<std::thread> pool;
	pool.reserve(T);
	for (int i = 0; i < T; ++i)
		pool.emplace_back(worker);
	for (auto &th : pool)
		th.join();

	std::ofstream out(path, std::ios::binary);
	out << "P6\n" << W << " " << H << "\n255\n";
	for (int y = 0; y < H; ++y)
	{
		for (int x = 0; x < W; ++x)
		{
			Vec3 c = framebuffer[y * W + x];
			c.x = std::clamp(c.x, 0.0, 1.0);
			c.y = std::clamp(c.y, 0.0, 1.0);
			c.z = std::clamp(c.z, 0.0, 1.0);
			unsigned char r =
				static_cast<unsigned char>(std::lround(c.x * 255.0));
			unsigned char g =
				static_cast<unsigned char>(std::lround(c.y * 255.0));
			unsigned char b =
				static_cast<unsigned char>(std::lround(c.z * 255.0));
			out.put(r).put(g).put(b);
		}
	}
}

void Renderer::render_window(std::vector<Material> &mats,
                                                         const RenderSettings &rset,
                                                         const std::string &scene_path)
{
        int W = rset.width;
        int H = rset.height;
        auto quality_scale = [](char q) -> float {
                if (q == 'M' || q == 'm')
                        return 1.5f;
                if (q == 'L' || q == 'l')
                        return 2.5f;
                return 1.0f;
        };
        float scale = std::max(1.0f, quality_scale(g_settings.quality));
        int RW = std::max(1, static_cast<int>(W / scale));
        int RH = std::max(1, static_cast<int>(H / scale));
        const int T = (rset.threads > 0)
                                          ? rset.threads
                                          : (std::thread::hardware_concurrency()
                                                         ? (int)std::thread::hardware_concurrency()
                                                         : 8);

        SDL_Window *win = nullptr;
        SDL_Renderer *ren = nullptr;
        SDL_Texture *tex = nullptr;
        if (!init_sdl(win, ren, tex, W, H, RW, RH))
                return;

        RenderState st;
        std::filesystem::path absolute_scene_path = std::filesystem::absolute(scene_path);
        st.scene_path = absolute_scene_path.string();
        st.level_paths = collect_level_paths(absolute_scene_path);
        st.current_level_index = level_index_for(st.level_paths, absolute_scene_path);
        st.cumulative_score = 0.0;
        st.player_name.clear();
        st.level_number = parse_level_number_from_path(st.scene_path);
        st.level_label = level_label_from_path(st.scene_path);
        st.focused = true;
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_ShowCursor(SDL_DISABLE);
        SDL_SetWindowGrab(win, SDL_TRUE);
        SDL_WarpMouseInWindow(win, W / 2, H / 2);

        std::vector<Vec3> framebuffer(RW * RH);
        std::vector<unsigned char> pixels(RW * RH * 3);
        Uint32 last = SDL_GetTicks();
        char current_quality = g_settings.quality;

        while (st.running)
        {
                Uint32 now = SDL_GetTicks();
                double dt = (now - last) / 1000.0;
                last = now;
                if (dt > 1e-6)
                        st.fps = 1.0 / dt;
                else
                        st.fps = 0.0;

                int actual_w = W;
                int actual_h = H;
                SDL_GetWindowSize(win, &actual_w, &actual_h);
                actual_w = std::max(1, actual_w);
                actual_h = std::max(1, actual_h);
                bool resolution_changed = (actual_w != W) || (actual_h != H);
                if (resolution_changed)
                {
                        W = actual_w;
                        H = actual_h;
                        cam.aspect = static_cast<double>(W) /
                                                 static_cast<double>(H);
                }

                bool quality_changed = false;
                if (g_settings.quality != current_quality)
                {
                        current_quality = g_settings.quality;
                        scale = std::max(1.0f, quality_scale(current_quality));
                        quality_changed = true;
                }

                if (quality_changed || resolution_changed)
                {
                        int new_RW = std::max(1, static_cast<int>(W / scale));
                        int new_RH = std::max(1, static_cast<int>(H / scale));
                        if (new_RW != RW || new_RH != RH)
                        {
                                SDL_Texture *new_tex =
                                        SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24,
                                                                          SDL_TEXTUREACCESS_STREAMING, new_RW,
                                                                          new_RH);
                                if (!new_tex)
                                        break;
                                SDL_DestroyTexture(tex);
                                tex = new_tex;
                                RW = new_RW;
                                RH = new_RH;
                        }
                        framebuffer.assign(RW * RH, Vec3());
                        pixels.assign(RW * RH * 3, 0);
                        if (resolution_changed && st.focused)
                                SDL_WarpMouseInWindow(win, W / 2, H / 2);
                }

                process_events(st, win, ren, W, H, mats);
                handle_keyboard(st, dt, mats);
                scene.update_goal_targets(dt, mats);
                update_selection(st, mats);
                if (st.scene_dirty)
                {
                        Uint32 now = SDL_GetTicks();
                        if (now - st.last_auto_save >= 100)
                        {
                                if (MapSaver::save(st.scene_path, scene, cam, mats))
                                {
                                        st.scene_dirty = false;
                                }
                                else
                                {
                                        std::cerr << "Failed to save scene to: " << st.scene_path
                                                  << "\n";
                                }
                                st.last_auto_save = now;
                        }
                }
                render_frame(st, ren, tex, framebuffer, pixels, RW, RH, W, H, T,
                                         mats);
        }

        SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
}
