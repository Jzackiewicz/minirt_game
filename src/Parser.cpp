#include "rt/Parser.h"
#include "rt/Sphere.h"
#include "rt/Plane.h"
#include "rt/Cylinder.h"
#include "rt/Cone.h"

#include <fstream>
#include <sstream>
#include <charconv>
#include <string_view>

namespace {

// helpery
inline void eat_ws(std::string_view& s) {
    while (!s.empty() && (s.front()==' ' || s.front()=='\t')) s.remove_prefix(1);
}
inline bool to_double(std::string_view sv, double& out) {
#if defined(__GLIBCXX__) && (__GLIBCXX__ < 20220225)
    // awaryjnie, gdy libstdc++ ma słabe from_chars dla double (stare systemy)
    char buf[128]; size_t n = std::min(sv.size(), sizeof(buf)-1);
    memcpy(buf, sv.data(), n); buf[n]=0;
    char* end=nullptr; out = std::strtod(buf, &end);
    return end!=buf;
#else
    const char* b = sv.data(); const char* e = sv.data()+sv.size();
    auto res = std::from_chars(b, e, out);
    return res.ec == std::errc{};
#endif
}
inline bool to_int(std::string_view sv, int& out) {
    const char* b = sv.data(); const char* e = sv.data()+sv.size();
    auto res = std::from_chars(b, e, out);
    return res.ec == std::errc{};
}
inline bool parse_triple(std::string_view sv, rt::Vec3& out) {
    eat_ws(sv);
    size_t p1 = sv.find(',');
    if (p1==std::string_view::npos) return false;
    size_t p2 = sv.find(',', p1+1);
    if (p2==std::string_view::npos) return false;
    auto s1 = sv.substr(0,p1);
    auto s2 = sv.substr(p1+1, p2-p1-1);
    auto s3 = sv.substr(p2+1);
    eat_ws(s1); eat_ws(s2); eat_ws(s3);
    double v1, v2, v3;
    if (!to_double(s1, v1) || !to_double(s2, v2) || !to_double(s3, v3)) return false;
    out = rt::Vec3(v1, v2, v3);
    return true;
}
inline rt::Vec3 rgb_to_unit(const rt::Vec3& rgb) {
    return rt::Vec3(rgb.x/255.0, rgb.y/255.0, rgb.z/255.0);
}

} // anon

namespace rt {

// storage dla materiałów
std::vector<Material> Parser::materials;

bool Parser::parse_rt_file(const std::string& path,
                           Scene& outScene,
                           Camera& outCamera,
                           int width, int height)
{
    std::ifstream in(path);
    if (!in) return false;

    std::string line;
    int oid = 0, mid = 0;

    outScene.ambient = Ambient(Vec3(1,1,1), 0.0);
    Vec3 cam_pos(0,0,-10), cam_dir(0,0,1);
    double fov = 60.0;

    while (std::getline(in, line)) {
        if (line.empty() || line[0]=='#') continue;
        std::istringstream iss(line);
        std::string id;
        iss >> id;

        if (id == "A") {
            std::string s_intens, s_rgb;
            iss >> s_intens >> s_rgb;
            double intensity=0;
            Vec3 rgb;
            if (to_double(s_intens, intensity) && parse_triple(s_rgb, rgb)) {
                outScene.ambient = Ambient(rgb_to_unit(rgb), intensity);
            }
        } else if (id == "C") {
            std::string s_pos, s_dir, s_fov;
            iss >> s_pos >> s_dir >> s_fov;
            parse_triple(s_pos, cam_pos);
            parse_triple(s_dir, cam_dir);
            to_double(s_fov, fov);
        } else if (id == "L") {
            std::string s_pos, s_intens, s_rgb;
            iss >> s_pos >> s_intens >> s_rgb;
            Vec3 p, rgb; double inten=1.0;
            if (parse_triple(s_pos, p) && to_double(s_intens, inten) && parse_triple(s_rgb, rgb)) {
                outScene.lights.emplace_back(p, rgb_to_unit(rgb), inten);
            }
        } else if (id == "sp") {
            std::string s_pos, s_r, s_rgb, s_mirror;
            iss >> s_pos >> s_r >> s_rgb >> s_mirror;
            Vec3 c, rgb; double r=1.0; int mir=0;
            if (parse_triple(s_pos, c) && to_double(s_r, r) && parse_triple(s_rgb, rgb)) {
                auto s = std::make_shared<Sphere>(c, r, oid++, mid);
                materials.emplace_back();
                materials.back().color = rgb_to_unit(rgb);
                outScene.objects.push_back(s);
                (void)mir; // na razie ignorujemy mirror flag
                ++mid;
            }
        } else if (id == "pl") {
            std::string s_p, s_n, s_rgb;
            iss >> s_p >> s_n >> s_rgb;
            Vec3 p, n, rgb;
            if (parse_triple(s_p, p) && parse_triple(s_n, n) && parse_triple(s_rgb, rgb)) {
                auto pl = std::make_shared<Plane>(p, n, oid++, mid);
                materials.emplace_back();
                materials.back().color = rgb_to_unit(rgb);
                outScene.objects.push_back(pl);
                ++mid;
            }
        } else if (id == "cy") {
            std::string s_pos, s_dir, s_d, s_h, s_rgb;
            iss >> s_pos >> s_dir >> s_d >> s_h >> s_rgb;
            Vec3 c, dir, rgb; double d = 1.0, h = 1.0;
            if (parse_triple(s_pos, c) && parse_triple(s_dir, dir)
                && to_double(s_d, d) && to_double(s_h, h)
                && parse_triple(s_rgb, rgb)) {
                auto cy = std::make_shared<Cylinder>(c, dir, d/2.0, h, oid++, mid);
                materials.emplace_back();
                materials.back().color = rgb_to_unit(rgb);
                outScene.objects.push_back(cy);
                ++mid;
            }
        } else if (id == "co") {
            std::string s_pos, s_dir, s_d, s_h, s_rgb;
            iss >> s_pos >> s_dir >> s_d >> s_h >> s_rgb;
            Vec3 c, dir, rgb; double d = 1.0, h = 1.0;
            if (parse_triple(s_pos, c) && parse_triple(s_dir, dir)
                && to_double(s_d, d) && to_double(s_h, h)
                && parse_triple(s_rgb, rgb)) {
                auto co = std::make_shared<Cone>(c, dir, d/2.0, h, oid++, mid);
                materials.emplace_back();
                materials.back().color = rgb_to_unit(rgb);
                outScene.objects.push_back(co);
                ++mid;
            }
        }
        // TODO: textures...
    }

    outCamera = Camera(cam_pos, cam_pos + cam_dir, fov, double(width)/double(height));
    return true;
}

const std::vector<Material>& Parser::get_materials() {
    return materials;
}

} // namespace rt
