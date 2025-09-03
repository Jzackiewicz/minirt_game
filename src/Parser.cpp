#include "rt/Parser.hpp"
#include "rt/Beam.hpp"
#include "rt/BeamSource.hpp"
#include "rt/Cube.hpp"
#include "rt/Cone.hpp"
#include "rt/Cylinder.hpp"
#include "rt/Plane.hpp"
#include "rt/Sphere.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string_view>
#include <cstring>
#include <filesystem>
#include <unordered_set>
#include <iomanip>

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
inline bool parse_triple(std::string_view sv, rt::Vec3 &out)
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
  out = rt::Vec3(v1, v2, v3);
  return true;
}
inline rt::Vec3 rgb_to_unit(const rt::Vec3 &rgb)
{
  return rt::Vec3(rgb.x / 255.0, rgb.y / 255.0, rgb.z / 255.0);
}
inline bool parse_rgba(std::string_view sv, rt::Vec3 &out, double &a)
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
  out = rt::Vec3(v1, v2, v3);
  a = v4;
  return true;
}
inline double alpha_to_unit(double a) { return a / 255.0; }

} // namespace

namespace rt
{

// storage dla materiałów
std::vector<Material> Parser::materials;

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
    {
      std::string s_intens, s_rgb;
      iss >> s_intens >> s_rgb;
      double intensity = 0;
      Vec3 rgb;
      double a = 255;
      if (to_double(s_intens, intensity) && parse_rgba(s_rgb, rgb, a))
      {
        outScene.ambient = Ambient(rgb_to_unit(rgb), intensity);
      }
    }
    else if (id == "C")
    {
      std::string s_pos, s_dir, s_fov;
      iss >> s_pos >> s_dir >> s_fov;
      parse_triple(s_pos, cam_pos);
      parse_triple(s_dir, cam_dir);
      to_double(s_fov, fov);
    }
    else if (id == "L")
    {
      std::string s_pos, s_intens, s_rgb;
      iss >> s_pos >> s_intens >> s_rgb;
      Vec3 p, rgb;
      double inten = 1.0;
      double a = 255;
      if (parse_triple(s_pos, p) && to_double(s_intens, inten) &&
          parse_rgba(s_rgb, rgb, a))
      {
        outScene.lights.emplace_back(p, rgb_to_unit(rgb), inten);
      }
    }
    else if (id == "sp")
    {
      std::string s_pos, s_r, s_rgb;
      iss >> s_pos >> s_r >> s_rgb;
      std::string s_mirror;
      if (!(iss >> s_mirror))
        s_mirror = "NR";
      std::string s_move;
      if (!(iss >> s_move))
        s_move = "IM";
      Vec3 c, rgb;
      double r = 1.0;
      double a = 255;
      if (parse_triple(s_pos, c) && to_double(s_r, r) &&
          parse_rgba(s_rgb, rgb, a))
      {
        auto s = std::make_shared<Sphere>(c, r, oid++, mid);
        s->movable = (s_move == "M");
        materials.emplace_back();
        materials.back().color = rgb_to_unit(rgb);
        materials.back().base_color = materials.back().color;
        materials.back().alpha = alpha_to_unit(a);
        materials.back().mirror = (s_mirror == "R" || s_mirror == "1");
        outScene.objects.push_back(s);
        ++mid;
      }
    }
    else if (id == "pl")
    {
      std::string s_p, s_n, s_rgb;
      iss >> s_p >> s_n >> s_rgb;
      std::string s_mirror;
      if (!(iss >> s_mirror))
        s_mirror = "NR";
      std::string s_move;
      if (!(iss >> s_move))
        s_move = "IM";
      Vec3 p, n, rgb;
      double a = 255;
      if (parse_triple(s_p, p) && parse_triple(s_n, n) &&
          parse_rgba(s_rgb, rgb, a))
      {
        auto pl = std::make_shared<Plane>(p, n, oid++, mid);
        pl->movable = (s_move == "M");
        materials.emplace_back();
        materials.back().color = rgb_to_unit(rgb);
        materials.back().base_color = materials.back().color;
        materials.back().alpha = alpha_to_unit(a);
        materials.back().mirror = (s_mirror == "R" || s_mirror == "1");
        outScene.objects.push_back(pl);
        ++mid;
      }
    }
    else if (id == "cy")
    {
      std::string s_pos, s_dir, s_d, s_h, s_rgb;
      iss >> s_pos >> s_dir >> s_d >> s_h >> s_rgb;
      std::string s_mirror;
      if (!(iss >> s_mirror))
        s_mirror = "NR";
      std::string s_move;
      if (!(iss >> s_move))
        s_move = "IM";
      Vec3 c, dir, rgb;
      double d = 1.0, h = 1.0;
      double a = 255;
      if (parse_triple(s_pos, c) && parse_triple(s_dir, dir) &&
          to_double(s_d, d) && to_double(s_h, h) && parse_rgba(s_rgb, rgb, a))
      {
        auto cy = std::make_shared<Cylinder>(c, dir, d / 2.0, h, oid++, mid);
        cy->movable = (s_move == "M");
        materials.emplace_back();
        materials.back().color = rgb_to_unit(rgb);
        materials.back().base_color = materials.back().color;
        materials.back().alpha = alpha_to_unit(a);
        materials.back().mirror = (s_mirror == "R" || s_mirror == "1");
        outScene.objects.push_back(cy);
        ++mid;
      }
    }
    else if (id == "cu")
    {
      std::string s_pos, s_orient, s_L, s_W, s_H, s_rgb;
      iss >> s_pos >> s_orient >> s_L >> s_W >> s_H >> s_rgb;
      std::string s_mirror;
      if (!(iss >> s_mirror))
        s_mirror = "NR";
      std::string s_move;
      if (!(iss >> s_move))
        s_move = "IM";
      Vec3 c, orient, rgb;
      double L = 1.0, W = 1.0, H = 1.0;
      double alpha = 255;
      if (parse_triple(s_pos, c) && parse_triple(s_orient, orient) &&
          to_double(s_L, L) && to_double(s_W, W) && to_double(s_H, H) &&
          parse_rgba(s_rgb, rgb, alpha))
      {
        auto cu = std::make_shared<Cube>(c, orient, L, W, H, oid++, mid);
        cu->movable = (s_move == "M");
        materials.emplace_back();
        materials.back().color = rgb_to_unit(rgb);
        materials.back().base_color = materials.back().color;
        materials.back().alpha = alpha_to_unit(alpha);
        materials.back().mirror = (s_mirror == "R" || s_mirror == "1");
        outScene.objects.push_back(cu);
        ++mid;
      }
    }
    else if (id == "bm")
    {
      std::string s_pos, s_dir, s_rgb, s_g, s_L;
      iss >> s_pos >> s_dir >> s_rgb >> s_g >> s_L;
      std::string s_move;
      if (!(iss >> s_move))
        s_move = "IM";
      std::string s_intens;
      if (!(iss >> s_intens))
        s_intens = "0.75";
      Vec3 o, dir, rgb;
      double g = 0.1, L = 1.0, intensity = 0.75;
      double a = 255;
      if (parse_triple(s_pos, o) && parse_triple(s_dir, dir) &&
          parse_rgba(s_rgb, rgb, a) && to_double(s_g, g) && to_double(s_L, L) &&
          to_double(s_intens, intensity))
      {
        Vec3 unit = rgb_to_unit(rgb);
        materials.emplace_back();
        materials.back().color = unit;
        materials.back().base_color = unit;
        materials.back().alpha = alpha_to_unit(a);
        materials.back().random_alpha = true;
        int beam_mat = mid++;

        Vec3 dir_norm = dir.normalized();
        auto bm = std::make_shared<Beam>(o, dir_norm, g, L, intensity,
                                         oid++, beam_mat);

        materials.emplace_back();
        materials.back().color = Vec3(1.0, 1.0, 1.0);
        materials.back().base_color = materials.back().color;
        materials.back().alpha = 0.25;
        int big_mat = mid++;

        materials.emplace_back();
        materials.back().color = (Vec3(1.0, 1.0, 1.0) + unit) * 0.5;
        materials.back().base_color = materials.back().color;
        materials.back().alpha = 0.5;
        int mid_mat = mid++;

        materials.emplace_back();
        materials.back().color = unit;
        materials.back().base_color = unit;
        materials.back().alpha = 1.0;
        int small_mat = mid++;

        auto src = std::make_shared<BeamSource>(o, dir_norm, bm, oid++,
                                                big_mat, mid_mat, small_mat);
        src->movable = (s_move == "M");
        bm->source = src;
        outScene.objects.push_back(bm);
        outScene.objects.push_back(src);
        double spot_ratio = 1.0;
        if (L > 1e-9)
          spot_ratio = std::clamp(g / L, 0.0, 1.0);
        double cone_cos = std::sqrt(std::max(0.0, 1.0 - spot_ratio * spot_ratio));
        outScene.lights.emplace_back(
            o, unit, intensity,
            std::vector<int>{bm->object_id, src->object_id, src->mid.object_id},
            src->object_id, dir_norm, cone_cos, L);
      }
    }
    else if (id == "co")
    {
      std::string s_pos, s_dir, s_d, s_h, s_rgb;
      iss >> s_pos >> s_dir >> s_d >> s_h >> s_rgb;
      std::string s_mirror;
      if (!(iss >> s_mirror))
        s_mirror = "NR";
      std::string s_move;
      if (!(iss >> s_move))
        s_move = "IM";
      Vec3 c, dir, rgb;
      double d = 1.0, h = 1.0;
      double a = 255;
      if (parse_triple(s_pos, c) && parse_triple(s_dir, dir) &&
          to_double(s_d, d) && to_double(s_h, h) && parse_rgba(s_rgb, rgb, a))
      {
        auto co = std::make_shared<Cone>(c, dir, d / 2.0, h, oid++, mid);
        co->movable = (s_move == "M");
        materials.emplace_back();
        materials.back().color = rgb_to_unit(rgb);
        materials.back().base_color = materials.back().color;
        materials.back().alpha = alpha_to_unit(a);
        materials.back().mirror = (s_mirror == "R" || s_mirror == "1");
        outScene.objects.push_back(co);
        ++mid;
      }
    }
    // TODO: textures...
  }

  outCamera =
      Camera(cam_pos, cam_pos + cam_dir, fov, double(width) / double(height));
  return true;
}

const std::vector<Material> &Parser::get_materials() { return materials; }

bool Parser::save_rt_file(const std::string &path, const Scene &scene,
                          const Camera &cam,
                          const std::vector<Material> &mats)
{
  std::ofstream out(path);
  if (!out)
    return false;

  auto vec_to_str = [](const Vec3 &v) {
    std::ostringstream oss;
    oss << v.x << ',' << v.y << ',' << v.z;
    return oss.str();
  };
  auto rgba_to_str = [](const Vec3 &c, double a) {
    std::ostringstream oss;
    int r = static_cast<int>(std::round(c.x * 255.0));
    int g = static_cast<int>(std::round(c.y * 255.0));
    int b = static_cast<int>(std::round(c.z * 255.0));
    int al = static_cast<int>(std::round(a * 255.0));
    oss << r << ',' << g << ',' << b << ',' << al;
    return oss.str();
  };

  out << "A " << scene.ambient.intensity << ' '
      << rgba_to_str(scene.ambient.color, 1.0) << '\n';

  out << "C " << vec_to_str(cam.origin) << ' ' << vec_to_str(cam.forward) << ' '
      << cam.fov_deg << '\n';

  for (const auto &L : scene.lights)
  {
    if (L.attached_id != -1)
      continue;
    out << "L " << vec_to_str(L.position) << ' ' << L.intensity << ' '
        << rgba_to_str(L.color, 1.0) << '\n';
  }

  std::unordered_set<int> beam_sources;
  for (const auto &obj : scene.objects)
  {
    if (obj->is_beam())
    {
      auto bm = std::static_pointer_cast<Beam>(obj);
      if (bm->start > 0.0)
        continue;
      if (auto src = bm->source.lock())
        beam_sources.insert(src->object_id);
      const Material &m = mats[bm->material_id];
      std::string move = "IM";
      if (auto src = bm->source.lock(); src && src->movable)
        move = "M";
      out << "bm " << vec_to_str(bm->path.orig) << ' '
          << vec_to_str(bm->path.dir) << ' '
          << rgba_to_str(m.base_color, m.alpha) << ' '
          << bm->radius << ' ' << bm->total_length << ' ' << move << ' '
          << bm->light_intensity << '\n';
    }
  }

  for (const auto &obj : scene.objects)
  {
    if (obj->is_beam())
      continue;
    if (beam_sources.count(obj->object_id))
      continue;
    const Material &m = mats[obj->material_id];
    std::string mirror = m.mirror ? "R" : "NR";
    std::string move = obj->movable ? "M" : "IM";
    switch (obj->shape_type())
    {
    case ShapeType::Sphere:
    {
      auto sp = static_cast<const Sphere *>(obj.get());
      out << "sp " << vec_to_str(sp->center) << ' ' << sp->radius << ' '
          << rgba_to_str(m.base_color, m.alpha) << ' ' << mirror << ' ' << move
          << '\n';
      break;
    }
    case ShapeType::Plane:
    {
      auto pl = static_cast<const Plane *>(obj.get());
      out << "pl " << vec_to_str(pl->point) << ' ' << vec_to_str(pl->normal)
          << ' ' << rgba_to_str(m.base_color, m.alpha) << ' ' << mirror << ' '
          << move << '\n';
      break;
    }
    case ShapeType::Cylinder:
    {
      auto cy = static_cast<const Cylinder *>(obj.get());
      out << "cy " << vec_to_str(cy->center) << ' ' << vec_to_str(cy->axis)
          << ' ' << (cy->radius * 2.0) << ' ' << cy->height << ' '
          << rgba_to_str(m.base_color, m.alpha) << ' ' << mirror << ' ' << move
          << '\n';
      break;
    }
    case ShapeType::Cube:
    {
      auto cu = static_cast<const Cube *>(obj.get());
      out << "cu " << vec_to_str(cu->center) << ' ' << vec_to_str(cu->axis[2])
          << ' ' << (cu->half.x * 2.0) << ' ' << (cu->half.y * 2.0) << ' '
          << (cu->half.z * 2.0) << ' ' << rgba_to_str(m.base_color, m.alpha)
          << ' ' << mirror << ' ' << move << '\n';
      break;
    }
    case ShapeType::Cone:
    {
      auto co = static_cast<const Cone *>(obj.get());
      out << "co " << vec_to_str(co->center) << ' ' << vec_to_str(co->axis)
          << ' ' << (co->radius * 2.0) << ' ' << co->height << ' '
          << rgba_to_str(m.base_color, m.alpha) << ' ' << mirror << ' ' << move
          << '\n';
      break;
    }
    default:
      break;
    }
  }

  return true;
}

} // namespace rt
