#include "Parser.hpp"
#include "Beam.hpp"
#include "BeamTarget.hpp"
#include "Cone.hpp"
#include "Cube.hpp"
#include "Cylinder.hpp"
#include "Plane.hpp"
#include "Sphere.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace
{

// helpery
inline void eat_ws(std::string_view &s)
{
	while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
		s.remove_prefix(1);
}
inline bool to_double(std::string_view sv, double &out)
{
#if defined(__GLIBCXX__) && (__GLIBCXX__ < 20220225)
	// awaryjnie, gdy libstdc++ ma słabe from_chars dla double (stare systemy)
	char buf[128];
	size_t n = std::min(sv.size(), sizeof(buf) - 1);
	std::memcpy(buf, sv.data(), n);
	buf[n] = 0;
	char *end = nullptr;
	out = std::strtod(buf, &end);
	return end != buf;
#else
	const char *b = sv.data();
	const char *e = sv.data() + sv.size();
	auto res = std::from_chars(b, e, out);
	return res.ec == std::errc{};
#endif
}
inline bool to_int(std::string_view sv, int &out)
{
	const char *b = sv.data();
	const char *e = sv.data() + sv.size();
	auto res = std::from_chars(b, e, out);
	return res.ec == std::errc{};
}
inline bool parse_triple(std::string_view sv, Vec3 &out)
{
	eat_ws(sv);
	size_t p1 = sv.find(',');
	if (p1 == std::string_view::npos)
		return false;
	size_t p2 = sv.find(',', p1 + 1);
	if (p2 == std::string_view::npos)
		return false;
	auto s1 = sv.substr(0, p1);
	auto s2 = sv.substr(p1 + 1, p2 - p1 - 1);
	auto s3 = sv.substr(p2 + 1);
	eat_ws(s1);
	eat_ws(s2);
	eat_ws(s3);
	double v1, v2, v3;
	if (!to_double(s1, v1) || !to_double(s2, v2) || !to_double(s3, v3))
		return false;
	out = Vec3(v1, v2, v3);
	return true;
}
inline Vec3 rgb_to_unit(const Vec3 &rgb)
{
	return Vec3(rgb.x / 255.0, rgb.y / 255.0, rgb.z / 255.0);
}
inline bool parse_rgba(std::string_view sv, Vec3 &out, double &a)
{
	eat_ws(sv);
	size_t p1 = sv.find(',');
	if (p1 == std::string_view::npos)
		return false;
	size_t p2 = sv.find(',', p1 + 1);
	if (p2 == std::string_view::npos)
		return false;
	size_t p3 = sv.find(',', p2 + 1);
	if (p3 == std::string_view::npos)
		return false;
	auto s1 = sv.substr(0, p1);
	auto s2 = sv.substr(p1 + 1, p2 - p1 - 1);
	auto s3 = sv.substr(p2 + 1, p3 - p2 - 1);
	auto s4 = sv.substr(p3 + 1);
	eat_ws(s1);
	eat_ws(s2);
	eat_ws(s3);
	eat_ws(s4);
	double v1, v2, v3, v4;
	if (!to_double(s1, v1) || !to_double(s2, v2) || !to_double(s3, v3) ||
		!to_double(s4, v4))
		return false;
	out = Vec3(v1, v2, v3);
	a = v4;
	return true;
}
inline double alpha_to_unit(double a) { return a / 255.0; }

} // namespace

// Parse ambient light definition line.
static void parse_ambient(std::istringstream &iss, Scene &scene)
{
        std::string s_intens, s_rgb;
        iss >> s_intens >> s_rgb;
        double intensity = 0;
        Vec3 rgb;
        double a = 255;
        if (to_double(s_intens, intensity) && parse_rgba(s_rgb, rgb, a))
                scene.ambient = Ambient(rgb_to_unit(rgb), intensity);
}

// Parse camera definition line.
static void parse_camera(std::istringstream &iss, Vec3 &pos, Vec3 &dir, double &fov)
{
        std::string s_pos, s_dir, s_fov;
        iss >> s_pos >> s_dir >> s_fov;
        parse_triple(s_pos, pos);
        parse_triple(s_dir, dir);
        to_double(s_fov, fov);
}

// Parse point light definition line.
static void parse_light(std::istringstream &iss, Scene &scene)
{
        std::string s_pos, s_intens, s_rgb;
        iss >> s_pos >> s_intens >> s_rgb;
        Vec3 p, rgb;
        double inten = 1.0;
        double a = 255;
        if (parse_triple(s_pos, p) && to_double(s_intens, inten) &&
                parse_rgba(s_rgb, rgb, a))
                scene.lights.emplace_back(p, rgb_to_unit(rgb), inten);
}

// Parse sphere definition line.
static void parse_sphere(std::istringstream &iss, Scene &scene, int &oid, int &mid,
                                               std::vector<Material> &mats)
{
        std::string s_pos, s_r, s_rgb;
        iss >> s_pos >> s_r >> s_rgb;
        std::string s_mirror;
        if (!(iss >> s_mirror))
                s_mirror = "NR";
        std::string s_move;
        if (!(iss >> s_move))
                s_move = "IM";
        std::string s_score;
        if (!(iss >> s_score))
                s_score = "S";
        Vec3 c, rgb;
        double r = 1.0;
        double a = 255;
        if (parse_triple(s_pos, c) && to_double(s_r, r) && parse_rgba(s_rgb, rgb, a))
        {
                auto s = std::make_shared<Sphere>(c, r, oid++, mid);
                s->movable = (s_move == "M");
                s->scorable = (s_score == "S");
                mats.emplace_back();
                mats.back().color = rgb_to_unit(rgb);
                mats.back().base_color = mats.back().color;
                mats.back().alpha = alpha_to_unit(a);
                mats.back().mirror = (s_mirror == "R" || s_mirror == "1");
                scene.objects.push_back(s);
                ++mid;
        }
}

// Parse plane definition line.
static void parse_plane(std::istringstream &iss, Scene &scene, int &oid, int &mid,
                                              std::vector<Material> &mats)
{
        std::string s_p, s_n, s_rgb;
        iss >> s_p >> s_n >> s_rgb;
        std::string s_mirror;
        if (!(iss >> s_mirror))
                s_mirror = "NR";
        std::string s_move;
        if (!(iss >> s_move))
                s_move = "IM";
        std::string s_score;
        if (!(iss >> s_score))
                s_score = "S";
        Vec3 p, n, rgb;
        double a = 255;
        if (parse_triple(s_p, p) && parse_triple(s_n, n) && parse_rgba(s_rgb, rgb, a))
        {
                auto pl = std::make_shared<Plane>(p, n, oid++, mid);
                pl->movable = (s_move == "M");
                pl->scorable = (s_score == "S");
                mats.emplace_back();
                mats.back().color = rgb_to_unit(rgb);
                mats.back().base_color = mats.back().color;
                mats.back().alpha = alpha_to_unit(a);
                mats.back().mirror = (s_mirror == "R" || s_mirror == "1");
                scene.objects.push_back(pl);
                ++mid;
        }
}

// Parse cylinder definition line.
static void parse_cylinder(std::istringstream &iss, Scene &scene, int &oid, int &mid,
                                                   std::vector<Material> &mats)
{
        std::string s_pos, s_dir, s_d, s_h, s_rgb;
        iss >> s_pos >> s_dir >> s_d >> s_h >> s_rgb;
        std::string s_mirror;
        if (!(iss >> s_mirror))
                s_mirror = "NR";
        std::string s_move;
        if (!(iss >> s_move))
                s_move = "IM";
        std::string s_score;
        if (!(iss >> s_score))
                s_score = "S";
        Vec3 c, dir, rgb;
        double d = 1.0, h = 1.0;
        double a = 255;
        if (parse_triple(s_pos, c) && parse_triple(s_dir, dir) &&
                to_double(s_d, d) && to_double(s_h, h) && parse_rgba(s_rgb, rgb, a))
        {
                auto cy =
                        std::make_shared<Cylinder>(c, dir, d / 2.0, h, oid++, mid);
                cy->movable = (s_move == "M");
                cy->scorable = (s_score == "S");
                mats.emplace_back();
                mats.back().color = rgb_to_unit(rgb);
                mats.back().base_color = mats.back().color;
                mats.back().alpha = alpha_to_unit(a);
                mats.back().mirror = (s_mirror == "R" || s_mirror == "1");
                scene.objects.push_back(cy);
                ++mid;
        }
}

// Parse cube definition line.
static void parse_cube(std::istringstream &iss, Scene &scene, int &oid, int &mid,
                                            std::vector<Material> &mats)
{
        std::string s_pos, s_orient, s_L, s_W, s_H, s_rgb;
        iss >> s_pos >> s_orient >> s_L >> s_W >> s_H >> s_rgb;
        std::string s_mirror;
        if (!(iss >> s_mirror))
                s_mirror = "NR";
        std::string s_move;
        if (!(iss >> s_move))
                s_move = "IM";
        std::string s_score;
        if (!(iss >> s_score))
                s_score = "S";
        Vec3 c, orient, rgb;
        double L = 1.0, W = 1.0, H = 1.0;
        double alpha = 255;
        if (parse_triple(s_pos, c) && parse_triple(s_orient, orient) &&
                to_double(s_L, L) && to_double(s_W, W) && to_double(s_H, H) &&
                parse_rgba(s_rgb, rgb, alpha))
        {
                auto cu =
                        std::make_shared<Cube>(c, orient, L, W, H, oid++, mid);
                cu->movable = (s_move == "M");
                cu->scorable = (s_score == "S");
                mats.emplace_back();
                mats.back().color = rgb_to_unit(rgb);
                mats.back().base_color = mats.back().color;
                mats.back().alpha = alpha_to_unit(alpha);
                mats.back().mirror = (s_mirror == "R" || s_mirror == "1");
                scene.objects.push_back(cu);
                ++mid;
        }
}

// Parse beam definition line.
static void parse_beam(std::istringstream &iss, Scene &scene, int &oid, int &mid,
                                            std::vector<Material> &mats)
{
       std::string s_intens, s_pos, s_dir, s_rgb, s_r, s_L;
       iss >> s_intens >> s_pos >> s_dir >> s_rgb >> s_r >> s_L;
        std::string s_move;
        if (!(iss >> s_move))
                s_move = "IM";
        std::string s_laser;
        if (!(iss >> s_laser))
                s_laser = "L";
        std::string s_score;
        if (!(iss >> s_score))
                s_score = "S";
       Vec3 o, dir, rgb;
       double ray_radius = 0.1, L = 1.0, intensity = 0.75;
        double a = 255;
        if (to_double(s_intens, intensity) && parse_triple(s_pos, o) &&
                parse_triple(s_dir, dir) && parse_rgba(s_rgb, rgb, a) &&
                to_double(s_r, ray_radius) && to_double(s_L, L))
        {
                Vec3 unit = rgb_to_unit(rgb);
                mats.emplace_back();
                mats.back().color = unit;
                mats.back().base_color = unit;
                mats.back().alpha = alpha_to_unit(a);
                mats.back().random_alpha = true;
                int beam_mat = mid++;

                Vec3 dir_norm = dir.normalized();

                mats.emplace_back();
                mats.back().color = Vec3(1.0, 1.0, 1.0);
                mats.back().base_color = mats.back().color;
                mats.back().alpha = 0.67;
                int big_mat = mid++;

                mats.emplace_back();
                mats.back().color = (Vec3(1.0, 1.0, 1.0) + unit) * 0.5;
                mats.back().base_color = mats.back().color;
                mats.back().alpha = 0.33;
                int mid_mat = mid++;

                mats.emplace_back();
                mats.back().color = unit;
                mats.back().base_color = unit;
                mats.back().alpha = 1.0;
                int small_mat = mid++;

               bool with_laser = (s_laser != "NL");
               auto bm = std::make_shared<Beam>(o, dir_norm, ray_radius, L,
                                                                                        intensity, oid, beam_mat,
                                                                                        big_mat, mid_mat, small_mat,
                                                                                        with_laser, unit);
                bool scorable = (s_score == "S");
                bool movable = (s_move == "M");
                bm->source->movable = movable;
                bm->source->scorable = scorable;
                bm->source->mid.scorable = scorable;
                bm->source->inner.scorable = scorable;
                if (bm->laser)
                        bm->laser->scorable = scorable;
                if (with_laser)
                {
                        oid += 2;
                        scene.objects.push_back(bm->laser);
                        scene.objects.push_back(bm->source);
                        const double cone_cos = std::sqrt(1.0 - 0.25 * 0.25);
                        scene.lights.emplace_back(
                                o, unit, intensity,
                                std::vector<int>{bm->laser->object_id,
                                                             bm->source->object_id,
                                                             bm->source->mid.object_id},
                                bm->source->object_id, dir_norm, cone_cos, L);
                }
                else
                {
                        oid += 1;
                        scene.objects.push_back(bm->source);
                        const double cone_cos = std::sqrt(1.0 - 0.25 * 0.25);
                        scene.lights.emplace_back(
                                o, unit, intensity,
                                std::vector<int>{bm->source->object_id,
                                                             bm->source->mid.object_id},
                                bm->source->object_id, dir_norm, cone_cos, L);
                }
        }
}

// Parse beam target definition line.
static void parse_beam_target(std::istringstream &iss, Scene &scene, int &oid,
                                                          int &mid, std::vector<Material> &mats)
{
        std::string s_pos, s_rgb, s_r;
        iss >> s_pos >> s_rgb >> s_r;
        std::string s_move;
        if (!(iss >> s_move))
                s_move = "IM";
        std::string s_score;
        if (!(iss >> s_score))
                s_score = "S";
        Vec3 c, rgb;
        double R = 1.0;
        double a = 255;
        if (parse_triple(s_pos, c) && parse_rgba(s_rgb, rgb, a) && to_double(s_r, R))
        {
                Vec3 unit = rgb_to_unit(rgb);

                mats.emplace_back();
                mats.back().color = Vec3(0.0, 0.0, 0.0);
                mats.back().base_color = mats.back().color;
                mats.back().alpha = 0.33;
                int big_mat = mid++;

                mats.emplace_back();
                mats.back().color = unit * 0.5;
                mats.back().base_color = mats.back().color;
                mats.back().alpha = 0.67;
                int mid_mat = mid++;

                mats.emplace_back();
                mats.back().color = unit;
                mats.back().base_color = unit;
                mats.back().alpha = 1.0;
                int small_mat = mid++;

                auto bt = std::make_shared<BeamTarget>(c, R, oid++, big_mat, mid_mat, small_mat);
                bt->movable = (s_move == "M");
                bt->scorable = (s_score == "S");
                bt->mid.scorable = bt->scorable;
                bt->inner.scorable = bt->scorable;
                scene.objects.push_back(bt);
        }
}

// Parse cone definition line.
static void parse_cone(std::istringstream &iss, Scene &scene, int &oid, int &mid,
                                           std::vector<Material> &mats)
{
        std::string s_pos, s_dir, s_d, s_h, s_rgb;
        iss >> s_pos >> s_dir >> s_d >> s_h >> s_rgb;
        std::string s_mirror;
        if (!(iss >> s_mirror))
                s_mirror = "NR";
        std::string s_move;
        if (!(iss >> s_move))
                s_move = "IM";
        std::string s_score;
        if (!(iss >> s_score))
                s_score = "S";
        Vec3 c, dir, rgb;
        double d = 1.0, h = 1.0;
        double a = 255;
        if (parse_triple(s_pos, c) && parse_triple(s_dir, dir) &&
                to_double(s_d, d) && to_double(s_h, h) && parse_rgba(s_rgb, rgb, a))
        {
                auto co =
                        std::make_shared<Cone>(c, dir, d / 2.0, h, oid++, mid);
                co->movable = (s_move == "M");
                co->scorable = (s_score == "S");
                mats.emplace_back();
                mats.back().color = rgb_to_unit(rgb);
                mats.back().base_color = mats.back().color;
                mats.back().alpha = alpha_to_unit(a);
                mats.back().mirror = (s_mirror == "R" || s_mirror == "1");
                scene.objects.push_back(co);
                ++mid;
        }
}

// storage dla materiałów
std::vector<Material> Parser::materials;

// Parse .rt file into scene and camera.
bool Parser::parse_rt_file(const std::string &path, Scene &outScene,
						   Camera &outCamera, int width, int height)
{
	std::ifstream in(path);
	if (!in)
		return false;

	std::string line;
	int oid = 0, mid = 0;

	outScene.ambient = Ambient(Vec3(1, 1, 1), 0.0);
	Vec3 cam_pos(0, 0, -10), cam_dir(0, 0, 1);
	double fov = 60.0;

        while (std::getline(in, line))
        {
                if (line.empty() || line[0] == '#')
                        continue;
                std::istringstream iss(line);
                std::string id;
                iss >> id;

                if (id == "A")
                        parse_ambient(iss, outScene);
                else if (id == "C")
                        parse_camera(iss, cam_pos, cam_dir, fov);
                else if (id == "L")
                        parse_light(iss, outScene);
                else if (id == "sp")
                        parse_sphere(iss, outScene, oid, mid, materials);
                else if (id == "pl")
                        parse_plane(iss, outScene, oid, mid, materials);
                else if (id == "cy")
                        parse_cylinder(iss, outScene, oid, mid, materials);
                else if (id == "cu")
                        parse_cube(iss, outScene, oid, mid, materials);
                else if (id == "bm")
                        parse_beam(iss, outScene, oid, mid, materials);
                else if (id == "bt")
                        parse_beam_target(iss, outScene, oid, mid, materials);
                else if (id == "co")
                        parse_cone(iss, outScene, oid, mid, materials);
                // TODO: textures...
        }

	outCamera =
		Camera(cam_pos, cam_pos + cam_dir, fov, double(width) / double(height));
	return true;
}

// Return materials parsed from last scene.
const std::vector<Material> &Parser::get_materials() { return materials; }

