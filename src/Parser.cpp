#include "Parser.hpp"
#include "Beam.hpp"
#include "BeamTarget.hpp"
#include "Cone.hpp"
#include "Cube.hpp"
#include "Cylinder.hpp"
#include "Plane.hpp"
#include "Sphere.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define SPOTLIGHT_LASER_RATIO 20.0
namespace
{

constexpr double kTransparentAlpha = 125.0 / 255.0;

std::string trim(const std::string &s)
{
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
                ++start;
        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
                --end;
        return s.substr(start, end - start);
}

std::string strip_comments(const std::string &line)
{
        std::string result;
        result.reserve(line.size());
        bool in_string = false;
        for (size_t i = 0; i < line.size(); ++i)
        {
                char c = line[i];
                if (c == '"' && (i == 0 || line[i - 1] != '\\'))
                        in_string = !in_string;
                if (!in_string && c == '#')
                        break;
                result.push_back(c);
        }
        return result;
}

bool to_double(std::string_view sv, double &out)
{
#if defined(__GLIBCXX__) && (__GLIBCXX__ < 20220225)
        char buf[128];
        size_t n = std::min(sv.size(), sizeof(buf) - 1);
        std::memcpy(buf, sv.data(), n);
        buf[n] = '\0';
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

bool to_int(std::string_view sv, int &out)
{
        const char *b = sv.data();
        const char *e = sv.data() + sv.size();
        auto res = std::from_chars(b, e, out);
        return res.ec == std::errc{};
}

bool report_error(size_t line, const std::string &message)
{
        std::cerr << "Parse error at line " << line << ": " << message << '\n';
        return false;
}

enum class TableType
{
        None,
        Camera,
        LightingAmbient,
        LightingLightSource,
        ObjectsPlane,
        ObjectsSphere,
        ObjectsCube,
        ObjectsCone,
        ObjectsCylinder,
        BeamSource,
        BeamTarget
};

const char *table_name(TableType type)
{
        switch (type)
        {
        case TableType::Camera:
                return "camera";
        case TableType::LightingAmbient:
                return "lighting.ambient";
        case TableType::LightingLightSource:
                return "lighting.light_sources";
        case TableType::ObjectsPlane:
                return "objects.planes";
        case TableType::ObjectsSphere:
                return "objects.spheres";
        case TableType::ObjectsCube:
                return "objects.boxes";
        case TableType::ObjectsCone:
                return "objects.cones";
        case TableType::ObjectsCylinder:
                return "objects.cylinders";
        case TableType::BeamSource:
                return "beam.sources";
        case TableType::BeamTarget:
                return "beam.targets";
        default:
                return "unknown";
        }
}

int table_stage(TableType type)
{
        switch (type)
        {
        case TableType::Camera:
                return 1;
        case TableType::LightingAmbient:
        case TableType::LightingLightSource:
                return 2;
        case TableType::ObjectsPlane:
        case TableType::ObjectsSphere:
        case TableType::ObjectsCube:
        case TableType::ObjectsCone:
        case TableType::ObjectsCylinder:
                return 3;
        case TableType::BeamSource:
        case TableType::BeamTarget:
                return 4;
        default:
                return 0;
        }
}

std::string table_header(TableType type)
{
        switch (type)
        {
        case TableType::Camera:
                return "[camera]";
        case TableType::LightingAmbient:
                return "[lighting.ambient]";
        case TableType::LightingLightSource:
                return "[[lighting.light_sources]]";
        case TableType::ObjectsPlane:
                return "[[objects.planes]]";
        case TableType::ObjectsSphere:
                return "[[objects.spheres]]";
        case TableType::ObjectsCube:
                return "[[objects.boxes]]";
        case TableType::ObjectsCone:
                return "[[objects.cones]]";
        case TableType::ObjectsCylinder:
                return "[[objects.cylinders]]";
        case TableType::BeamSource:
                return "[[beam.sources]]";
        case TableType::BeamTarget:
                return "[[beam.targets]]";
        default:
                return "[unknown]";
        }
}

struct TableData
{
        TableType type = TableType::None;
        size_t header_line = 0;
        std::unordered_map<std::string, std::pair<std::string, size_t>> values;
};

bool key_allowed(const std::string &key, std::initializer_list<const char *> allowed)
{
        for (const char *entry : allowed)
                if (key == entry)
                        return true;
        return false;
}

bool check_allowed_keys(const TableData &table, std::initializer_list<const char *> allowed)
{
        for (const auto &kv : table.values)
                if (!key_allowed(kv.first, allowed))
                        return report_error(kv.second.second,
                                            "Unexpected key '" + kv.first + "' in " + table_header(table.type));
        return true;
}

bool require_value(const TableData &table, const std::string &key, std::string &out, size_t &line)
{
        auto it = table.values.find(key);
        if (it == table.values.end())
                return report_error(table.header_line, "Missing key '" + key + "' in " + table_header(table.type));
        out = it->second.first;
        line = it->second.second;
        return true;
}

bool parse_array(const std::string &raw, std::vector<std::string> &out, size_t expected)
{
        if (raw.size() < 2 || raw.front() != '[' || raw.back() != ']')
                return false;
        std::string inner = raw.substr(1, raw.size() - 2);
        out.clear();
        size_t pos = 0;
        while (pos <= inner.size())
        {
                size_t next = inner.find(',', pos);
                if (next == std::string::npos)
                        next = inner.size();
                std::string part = trim(inner.substr(pos, next - pos));
                if (part.empty())
                        return false;
                out.push_back(part);
                pos = next + 1;
        }
        return out.size() == expected;
}

bool parse_vec3_field(const TableData &table, const std::string &key, Vec3 &out)
{
        std::string raw;
        size_t line = table.header_line;
        if (!require_value(table, key, raw, line))
                return false;
        std::vector<std::string> parts;
        if (!parse_array(raw, parts, 3))
                return report_error(line, "Expected array of three numbers for '" + key + "'");
        double values[3];
        for (int i = 0; i < 3; ++i)
        {
                std::string_view sv(parts[i]);
                if (!to_double(sv, values[i]) || !std::isfinite(values[i]))
                        return report_error(line, "Invalid number in '" + key + "'");
        }
        out = Vec3(values[0], values[1], values[2]);
        return true;
}

bool parse_color_field(const TableData &table, const std::string &key, std::array<int, 3> &out)
{
        std::string raw;
        size_t line = table.header_line;
        if (!require_value(table, key, raw, line))
                return false;
        std::vector<std::string> parts;
        if (!parse_array(raw, parts, 3))
                return report_error(line, "Expected array of three integers for '" + key + "'");
        for (int i = 0; i < 3; ++i)
        {
                std::string_view sv(parts[i]);
                if (!to_int(sv, out[i]))
                        return report_error(line, "Invalid integer in '" + key + "'");
                if (out[i] < 0 || out[i] > 255)
                        return report_error(line, "Color component out of range in '" + key + "'");
        }
        return true;
}

bool parse_double_field(const TableData &table, const std::string &key, double &out)
{
        std::string raw;
        size_t line = table.header_line;
        if (!require_value(table, key, raw, line))
                return false;
        std::string trimmed = trim(raw);
        std::string_view sv(trimmed);
        if (!to_double(sv, out) || !std::isfinite(out))
                return report_error(line, "Invalid number for '" + key + "'");
        return true;
}

bool parse_positive_double_field(const TableData &table, const std::string &key, double &out)
{
        if (!parse_double_field(table, key, out))
                return false;
        if (!(out > 0.0))
                return report_error(table.values.at(key).second,
                                    "Value for '" + key + "' must be positive");
        return true;
}

bool parse_non_negative_double_field(const TableData &table, const std::string &key, double &out)
{
        if (!parse_double_field(table, key, out))
                return false;
        if (out < 0.0)
                return report_error(table.values.at(key).second,
                                    "Value for '" + key + "' must be non-negative");
        return true;
}

bool parse_double_range_field(const TableData &table, const std::string &key, double &out,
                              double min, double max)
{
        if (!parse_double_field(table, key, out))
                return false;
        if (out < min || out > max)
                return report_error(table.values.at(key).second,
                                    "Value for '" + key + "' out of allowed range");
        return true;
}

bool parse_bool_field(const TableData &table, const std::string &key, bool &out)
{
        std::string raw;
        size_t line = table.header_line;
        if (!require_value(table, key, raw, line))
                return false;
        std::string trimmed = trim(raw);
        std::string lower;
        lower.reserve(trimmed.size());
        for (char c : trimmed)
                lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        if (lower == "true")
                out = true;
        else if (lower == "false")
                out = false;
        else
                return report_error(line, "Expected boolean for '" + key + "'");
        return true;
}

bool ensure_non_zero(const Vec3 &v, size_t line, const std::string &field, TableType type)
{
        if (v.length_squared() == 0.0)
                return report_error(line, "Zero vector not allowed for '" + field + "' in " + table_header(type));
        if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z))
                return report_error(line, "Non-finite vector in '" + field + "'");
        return true;
}

Vec3 rgb_to_unit(const std::array<int, 3> &rgb)
{
        return Vec3(rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0);
}

bool parse_string_field(const TableData &table, const std::string &key, std::string &out)
{
        std::string raw;
        size_t line = table.header_line;
        if (!require_value(table, key, raw, line))
                return false;
        std::string trimmed = trim(raw);
        if (trimmed.size() < 2 || trimmed.front() != '"' || trimmed.back() != '"')
                return report_error(line, "Expected string for '" + key + "'");
        out = trimmed.substr(1, trimmed.size() - 2);
        return true;
}

Material make_material(const std::array<int, 3> &rgb, bool reflective, bool transparent)
{
        Material mat;
        Vec3 color = rgb_to_unit(rgb);
        mat.color = color;
        mat.base_color = color;
        mat.alpha = transparent ? kTransparentAlpha : 1.0;
        mat.mirror = reflective;
        return mat;
}

bool process_camera(const TableData &table, Vec3 &cam_pos, Vec3 &cam_dir, double &fov)
{
        if (!check_allowed_keys(table, {"id", "position", "lookdir", "fov"}))
                return false;
        std::string id;
        if (!parse_string_field(table, "id", id))
                return false;
        if (id.empty())
                return report_error(table.values.at("id").second, "Camera id cannot be empty");
        Vec3 position;
        if (!parse_vec3_field(table, "position", position))
                return false;
        Vec3 lookdir;
        if (!parse_vec3_field(table, "lookdir", lookdir))
                return false;
        if (!ensure_non_zero(lookdir, table.values.at("lookdir").second, "lookdir", table.type))
                return false;
        if (!parse_double_range_field(table, "fov", fov, 0.0, 180.0))
                return false;
        if (!(fov > 0.0 && fov < 180.0))
                return report_error(table.values.at("fov").second, "Camera FOV must be in (0, 180)");
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
                return report_error(table.values.at("position").second, "Camera position must be finite");
        cam_pos = position;
        cam_dir = lookdir.normalized();
        return true;
}

bool process_lighting_ambient(const TableData &table, Scene &scene)
{
        if (!check_allowed_keys(table, {"intensity", "color"}))
                return false;
        double intensity;
        if (!parse_double_range_field(table, "intensity", intensity, 0.0, 1.0))
                return false;
        std::array<int, 3> rgb;
        if (!parse_color_field(table, "color", rgb))
                return false;
        scene.ambient = Ambient(rgb_to_unit(rgb), intensity);
        return true;
}

bool process_lighting_light_source(const TableData &table, Scene &scene,
                                   std::unordered_set<std::string> &light_ids)
{
        if (!check_allowed_keys(table, {"id", "intensity", "position", "color"}))
                return false;
        std::string id;
        if (!parse_string_field(table, "id", id))
                return false;
        if (id.empty())
                return report_error(table.values.at("id").second, "Light id cannot be empty");
        if (!light_ids.insert(id).second)
                return report_error(table.values.at("id").second, "Duplicate light id '" + id + "'");
        double intensity;
        if (!parse_non_negative_double_field(table, "intensity", intensity))
                return false;
        Vec3 position;
        if (!parse_vec3_field(table, "position", position))
                return false;
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
                return report_error(table.values.at("position").second, "Light position must be finite");
        std::array<int, 3> rgb;
        if (!parse_color_field(table, "color", rgb))
                return false;
        scene.lights.emplace_back(position, rgb_to_unit(rgb), intensity);
        return true;
}

bool process_plane(const TableData &table, Scene &scene, int &oid, int &mid,
                   std::vector<Material> &materials, std::unordered_set<std::string> &object_ids)
{
        if (!check_allowed_keys(table,
                                {"id", "color", "position", "dir", "reflective", "rotatable",
                                 "movable", "scorable", "transparent"}))
                return false;
        std::string id;
        if (!parse_string_field(table, "id", id))
                return false;
        if (!object_ids.insert(id).second)
                return report_error(table.values.at("id").second, "Duplicate object id '" + id + "'");
        std::array<int, 3> rgb;
        if (!parse_color_field(table, "color", rgb))
                return false;
        Vec3 position;
        if (!parse_vec3_field(table, "position", position))
                return false;
        Vec3 normal;
        if (!parse_vec3_field(table, "dir", normal))
                return false;
        size_t dir_line = table.values.at("dir").second;
        if (!ensure_non_zero(normal, dir_line, "dir", table.type))
                return false;
        bool reflective;
        if (!parse_bool_field(table, "reflective", reflective))
                return false;
        bool rotatable;
        if (!parse_bool_field(table, "rotatable", rotatable))
                return false;
        if (rotatable)
                return report_error(table.values.at("rotatable").second,
                                    "Planes cannot be rotatable");
        bool movable;
        if (!parse_bool_field(table, "movable", movable))
                return false;
        bool scorable;
        if (!parse_bool_field(table, "scorable", scorable))
                return false;
        bool transparent;
        if (!parse_bool_field(table, "transparent", transparent))
                return false;
        auto plane = std::make_shared<Plane>(position, normal.normalized(), oid++, mid);
        plane->movable = movable;
        plane->scorable = scorable;
        materials.push_back(make_material(rgb, reflective, transparent));
        scene.objects.push_back(plane);
        ++mid;
        return true;
}

bool process_sphere(const TableData &table, Scene &scene, int &oid, int &mid,
                    std::vector<Material> &materials, std::unordered_set<std::string> &object_ids)
{
        if (!check_allowed_keys(table,
                                {"id", "color", "position", "dir", "radius", "reflective",
                                 "rotatable", "movable", "scorable", "transparent"}))
                return false;
        std::string id;
        if (!parse_string_field(table, "id", id))
                return false;
        if (!object_ids.insert(id).second)
                return report_error(table.values.at("id").second, "Duplicate object id '" + id + "'");
        std::array<int, 3> rgb;
        if (!parse_color_field(table, "color", rgb))
                return false;
        Vec3 position;
        if (!parse_vec3_field(table, "position", position))
                return false;
        Vec3 dummy_dir;
        if (!parse_vec3_field(table, "dir", dummy_dir))
                return false;
        double radius;
        if (!parse_positive_double_field(table, "radius", radius))
                return false;
        bool reflective;
        if (!parse_bool_field(table, "reflective", reflective))
                return false;
        bool rotatable;
        if (!parse_bool_field(table, "rotatable", rotatable))
                return false;
        bool movable;
        if (!parse_bool_field(table, "movable", movable))
                return false;
        bool scorable;
        if (!parse_bool_field(table, "scorable", scorable))
                return false;
        bool transparent;
        if (!parse_bool_field(table, "transparent", transparent))
                return false;
        auto sphere = std::make_shared<Sphere>(position, radius, oid++, mid);
        sphere->rotatable = rotatable;
        sphere->movable = movable;
        sphere->scorable = scorable;
        materials.push_back(make_material(rgb, reflective, transparent));
        scene.objects.push_back(sphere);
        ++mid;
        return true;
}

bool process_cube(const TableData &table, Scene &scene, int &oid, int &mid,
                  std::vector<Material> &materials, std::unordered_set<std::string> &object_ids)
{
        if (!check_allowed_keys(table,
                                {"id", "color", "position", "dir", "width", "height", "length",
                                 "reflective", "rotatable", "movable", "scorable", "transparent"}))
                return false;
        std::string id;
        if (!parse_string_field(table, "id", id))
                return false;
        if (!object_ids.insert(id).second)
                return report_error(table.values.at("id").second, "Duplicate object id '" + id + "'");
        std::array<int, 3> rgb;
        if (!parse_color_field(table, "color", rgb))
                return false;
        Vec3 position;
        if (!parse_vec3_field(table, "position", position))
                return false;
        Vec3 dir;
        if (!parse_vec3_field(table, "dir", dir))
                return false;
        size_t dir_line = table.values.at("dir").second;
        if (!ensure_non_zero(dir, dir_line, "dir", table.type))
                return false;
        double width;
        if (!parse_positive_double_field(table, "width", width))
                return false;
        double height;
        if (!parse_positive_double_field(table, "height", height))
                return false;
        double length;
        if (!parse_positive_double_field(table, "length", length))
                return false;
        bool reflective;
        if (!parse_bool_field(table, "reflective", reflective))
                return false;
        bool rotatable;
        if (!parse_bool_field(table, "rotatable", rotatable))
                return false;
        bool movable;
        if (!parse_bool_field(table, "movable", movable))
                return false;
        bool scorable;
        if (!parse_bool_field(table, "scorable", scorable))
                return false;
        bool transparent;
        if (!parse_bool_field(table, "transparent", transparent))
                return false;
        auto cube = std::make_shared<Cube>(position, dir.normalized(), length, width, height, oid++, mid);
        cube->rotatable = rotatable;
        cube->movable = movable;
        cube->scorable = scorable;
        materials.push_back(make_material(rgb, reflective, transparent));
        scene.objects.push_back(cube);
        ++mid;
        return true;
}

bool process_cylinder(const TableData &table, Scene &scene, int &oid, int &mid,
                      std::vector<Material> &materials, std::unordered_set<std::string> &object_ids)
{
        if (!check_allowed_keys(table,
                                {"id", "color", "position", "dir", "radius", "height",
                                 "reflective", "rotatable", "movable", "scorable", "transparent"}))
                return false;
        std::string id;
        if (!parse_string_field(table, "id", id))
                return false;
        if (!object_ids.insert(id).second)
                return report_error(table.values.at("id").second, "Duplicate object id '" + id + "'");
        std::array<int, 3> rgb;
        if (!parse_color_field(table, "color", rgb))
                return false;
        Vec3 position;
        if (!parse_vec3_field(table, "position", position))
                return false;
        Vec3 dir;
        if (!parse_vec3_field(table, "dir", dir))
                return false;
        size_t dir_line = table.values.at("dir").second;
        if (!ensure_non_zero(dir, dir_line, "dir", table.type))
                return false;
        double radius;
        if (!parse_positive_double_field(table, "radius", radius))
                return false;
        double height;
        if (!parse_positive_double_field(table, "height", height))
                return false;
        bool reflective;
        if (!parse_bool_field(table, "reflective", reflective))
                return false;
        bool rotatable;
        if (!parse_bool_field(table, "rotatable", rotatable))
                return false;
        bool movable;
        if (!parse_bool_field(table, "movable", movable))
                return false;
        bool scorable;
        if (!parse_bool_field(table, "scorable", scorable))
                return false;
        bool transparent;
        if (!parse_bool_field(table, "transparent", transparent))
                return false;
        auto cylinder = std::make_shared<Cylinder>(position, dir.normalized(), radius, height, oid++, mid);
        cylinder->rotatable = rotatable;
        cylinder->movable = movable;
        cylinder->scorable = scorable;
        materials.push_back(make_material(rgb, reflective, transparent));
        scene.objects.push_back(cylinder);
        ++mid;
        return true;
}

bool process_cone(const TableData &table, Scene &scene, int &oid, int &mid,
                  std::vector<Material> &materials, std::unordered_set<std::string> &object_ids)
{
        if (!check_allowed_keys(table,
                                {"id", "color", "position", "dir", "radius", "height",
                                 "reflective", "rotatable", "movable", "scorable", "transparent"}))
                return false;
        std::string id;
        if (!parse_string_field(table, "id", id))
                return false;
        if (!object_ids.insert(id).second)
                return report_error(table.values.at("id").second, "Duplicate object id '" + id + "'");
        std::array<int, 3> rgb;
        if (!parse_color_field(table, "color", rgb))
                return false;
        Vec3 position;
        if (!parse_vec3_field(table, "position", position))
                return false;
        Vec3 dir;
        if (!parse_vec3_field(table, "dir", dir))
                return false;
        size_t dir_line = table.values.at("dir").second;
        if (!ensure_non_zero(dir, dir_line, "dir", table.type))
                return false;
        double radius;
        if (!parse_positive_double_field(table, "radius", radius))
                return false;
        double height;
        if (!parse_positive_double_field(table, "height", height))
                return false;
        bool reflective;
        if (!parse_bool_field(table, "reflective", reflective))
                return false;
        bool rotatable;
        if (!parse_bool_field(table, "rotatable", rotatable))
                return false;
        bool movable;
        if (!parse_bool_field(table, "movable", movable))
                return false;
        bool scorable;
        if (!parse_bool_field(table, "scorable", scorable))
                return false;
        bool transparent;
        if (!parse_bool_field(table, "transparent", transparent))
                return false;
        auto cone = std::make_shared<Cone>(position, dir.normalized(), radius, height, oid++, mid);
        cone->rotatable = rotatable;
        cone->movable = movable;
        cone->scorable = scorable;
        materials.push_back(make_material(rgb, reflective, transparent));
        scene.objects.push_back(cone);
        ++mid;
        return true;
}

bool process_beam_source(const TableData &table, Scene &scene, int &oid, int &mid,
                         std::vector<Material> &materials,
                         std::unordered_set<std::string> &beam_ids)
{
        if (!check_allowed_keys(table,
                                {"id", "intensity", "position", "dir", "color", "radius", "length",
                                 "movable", "rotatable", "scorable", "with_laser"}))
                return false;
        std::string id;
        if (!parse_string_field(table, "id", id))
                return false;
        if (!beam_ids.insert(id).second)
                return report_error(table.values.at("id").second, "Duplicate beam source id '" + id + "'");
        double intensity;
        if (!parse_non_negative_double_field(table, "intensity", intensity))
                return false;
        Vec3 position;
        if (!parse_vec3_field(table, "position", position))
                return false;
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
                return report_error(table.values.at("position").second, "Beam source position must be finite");
        Vec3 dir;
        if (!parse_vec3_field(table, "dir", dir))
                return false;
        size_t dir_line = table.values.at("dir").second;
        if (!ensure_non_zero(dir, dir_line, "dir", table.type))
                return false;
        std::array<int, 3> rgb;
        if (!parse_color_field(table, "color", rgb))
                return false;
        double source_radius;
        if (!parse_positive_double_field(table, "radius", source_radius))
                return false;
        double length;
        if (!parse_positive_double_field(table, "length", length))
                return false;
        bool movable;
        if (!parse_bool_field(table, "movable", movable))
                return false;
        bool rotatable;
        if (!parse_bool_field(table, "rotatable", rotatable))
                return false;
        bool scorable;
        if (!parse_bool_field(table, "scorable", scorable))
                return false;
        bool with_laser;
        if (!parse_bool_field(table, "with_laser", with_laser))
                return false;
        Vec3 color_unit = rgb_to_unit(rgb);
        double beam_radius = source_radius * 0.5;

        materials.emplace_back();
        materials.back().color = color_unit;
        materials.back().base_color = color_unit;
        materials.back().alpha = kTransparentAlpha;
        materials.back().random_alpha = true;
        int beam_mat = mid++;

        Vec3 dir_norm = dir.normalized();

        materials.emplace_back();
        materials.back().color = Vec3(1.0, 1.0, 1.0);
        materials.back().base_color = materials.back().color;
        materials.back().alpha = 0.67;
        int big_mat = mid++;

        materials.emplace_back();
        materials.back().color = (Vec3(1.0, 1.0, 1.0) + color_unit) * 0.5;
        materials.back().base_color = materials.back().color;
        materials.back().alpha = 0.33;
        int mid_mat = mid++;

        materials.emplace_back();
        materials.back().color = color_unit;
        materials.back().base_color = color_unit;
        materials.back().alpha = 1.0;
        int small_mat = mid++;

        auto beam = std::make_shared<Beam>(position, dir_norm, beam_radius, length, intensity,
                                           oid, beam_mat, big_mat, mid_mat, small_mat, with_laser, color_unit);
        bool movable_flag = movable;
        bool scorable_flag = scorable;
        beam->source->movable = movable_flag;
        beam->source->rotatable = rotatable;
        beam->source->scorable = scorable_flag;
        beam->source->mid.scorable = scorable_flag;
        beam->source->inner.scorable = scorable_flag;
        if (beam->laser)
        {
                beam->laser->scorable = scorable_flag;
                beam->laser->rotatable = rotatable;
        }
        const double cone_cos = std::sqrt(1.0 - 0.25 * 0.25);
        double spot_radius = 0.0;
        if (beam->laser)
                spot_radius = beam->laser->radius * SPOTLIGHT_LASER_RATIO;
        else if (beam->light)
                spot_radius = beam->light->radius * SPOTLIGHT_LASER_RATIO;
        else
                spot_radius = beam_radius * SPOTLIGHT_LASER_RATIO;
        if (with_laser)
        {
                oid += 2;
                scene.objects.push_back(beam->laser);
                scene.objects.push_back(beam->source);
                scene.lights.emplace_back(position, color_unit, intensity,
                                          std::vector<int>{beam->laser->object_id,
                                                           beam->source->object_id,
                                                           beam->source->mid.object_id},
                                          beam->source->object_id, dir_norm, cone_cos, length,
                                          false, true, spot_radius);
        }
        else
        {
                oid += 1;
                scene.objects.push_back(beam->source);
                scene.lights.emplace_back(position, color_unit, intensity,
                                          std::vector<int>{beam->source->object_id,
                                                           beam->source->mid.object_id},
                                          beam->source->object_id, dir_norm, cone_cos, length,
                                          false, true, spot_radius);
        }
        return true;
}

bool process_beam_target(const TableData &table, Scene &scene, int &oid, int &mid,
                         std::vector<Material> &materials,
                         std::unordered_set<std::string> &target_ids)
{
        if (!check_allowed_keys(table,
                                {"id", "position", "color", "radius", "movable", "scorable"}))
                return false;
        std::string id;
        if (!parse_string_field(table, "id", id))
                return false;
        if (!target_ids.insert(id).second)
                return report_error(table.values.at("id").second, "Duplicate beam target id '" + id + "'");
        Vec3 position;
        if (!parse_vec3_field(table, "position", position))
                return false;
        double radius;
        if (!parse_positive_double_field(table, "radius", radius))
                return false;
        std::array<int, 3> rgb;
        if (!parse_color_field(table, "color", rgb))
                return false;
        bool movable;
        if (!parse_bool_field(table, "movable", movable))
                return false;
        bool scorable;
        if (!parse_bool_field(table, "scorable", scorable))
                return false;

        Vec3 color_unit = rgb_to_unit(rgb);

        materials.emplace_back();
        materials.back().color = Vec3(0.0, 0.0, 0.0);
        materials.back().base_color = materials.back().color;
        materials.back().alpha = 0.33;
        int big_mat = mid++;

        materials.emplace_back();
        materials.back().color = color_unit * 0.5;
        materials.back().base_color = materials.back().color;
        materials.back().alpha = 0.67;
        int mid_mat = mid++;

        materials.emplace_back();
        materials.back().color = color_unit;
        materials.back().base_color = color_unit;
        materials.back().alpha = 1.0;
        int small_mat = mid++;

        auto target = std::make_shared<BeamTarget>(position, radius, oid++, big_mat, mid_mat, small_mat);
        target->movable = movable;
        target->scorable = scorable;
        target->mid.scorable = scorable;
        target->inner.scorable = scorable;
        scene.objects.push_back(target);
        return true;
}

} // namespace

std::vector<Material> Parser::materials;

bool Parser::parse_rt_file(const std::string &path, Scene &outScene,
                                                   Camera &outCamera, int width, int height)
{
        std::ifstream in(path);
        if (!in)
        {
                std::cerr << "Failed to open scene file: " << path << '\n';
                return false;
        }

        materials.clear();
        outScene.objects.clear();
        outScene.lights.clear();
        outScene.accel.reset();
        outScene.ambient = Ambient(Vec3(1, 1, 1), 0.0);

        Vec3 cam_pos(0, 0, -10);
        Vec3 cam_dir(0, 0, 1);
        double fov = 60.0;
        bool camera_seen = false;
        bool ambient_seen = false;
        bool camera_declared = false;
        bool ambient_declared = false;

        int oid = 0;
        int mid = 0;

        std::unordered_set<std::string> object_ids;
        std::unordered_set<std::string> light_ids;
        std::unordered_set<std::string> beam_source_ids;
        std::unordered_set<std::string> beam_target_ids;

        TableData current;
        size_t line_no = 0;
        int highest_stage = 0;

        auto finalize_table = [&](TableData &table) -> bool
        {
                if (table.type == TableType::None)
                        return true;
                bool ok = false;
                switch (table.type)
                {
                case TableType::Camera:
                        ok = process_camera(table, cam_pos, cam_dir, fov);
                        camera_seen = ok;
                        break;
                case TableType::LightingAmbient:
                        ok = process_lighting_ambient(table, outScene);
                        ambient_seen = ambient_seen || ok;
                        break;
                case TableType::LightingLightSource:
                        ok = process_lighting_light_source(table, outScene, light_ids);
                        break;
                case TableType::ObjectsPlane:
                        ok = process_plane(table, outScene, oid, mid, materials, object_ids);
                        break;
                case TableType::ObjectsSphere:
                        ok = process_sphere(table, outScene, oid, mid, materials, object_ids);
                        break;
                case TableType::ObjectsCube:
                        ok = process_cube(table, outScene, oid, mid, materials, object_ids);
                        break;
                case TableType::ObjectsCone:
                        ok = process_cone(table, outScene, oid, mid, materials, object_ids);
                        break;
                case TableType::ObjectsCylinder:
                        ok = process_cylinder(table, outScene, oid, mid, materials, object_ids);
                        break;
                case TableType::BeamSource:
                        ok = process_beam_source(table, outScene, oid, mid, materials, beam_source_ids);
                        break;
                case TableType::BeamTarget:
                        ok = process_beam_target(table, outScene, oid, mid, materials, beam_target_ids);
                        break;
                default:
                        ok = false;
                        break;
                }
                table = TableData{};
                return ok;
        };

        std::string raw_line;
        while (std::getline(in, raw_line))
        {
                ++line_no;
                std::string line = strip_comments(raw_line);
                line = trim(line);
                if (line.empty())
                        continue;
                if (line.front() == '[')
                {
                        if (!finalize_table(current))
                                return false;
                        TableType next_type = TableType::None;
                        if (line.size() >= 4 && line[0] == '[' && line[1] == '[' &&
                            line[line.size() - 1] == ']' && line[line.size() - 2] == ']')
                        {
                                std::string name = trim(line.substr(2, line.size() - 4));
                                if (name == "lighting.light_sources")
                                        next_type = TableType::LightingLightSource;
                                else if (name == "objects.planes")
                                        next_type = TableType::ObjectsPlane;
                                else if (name == "objects.spheres")
                                        next_type = TableType::ObjectsSphere;
                                else if (name == "objects.boxes")
                                        next_type = TableType::ObjectsCube;
                                else if (name == "objects.cones")
                                        next_type = TableType::ObjectsCone;
                                else if (name == "objects.cylinders")
                                        next_type = TableType::ObjectsCylinder;
                                else if (name == "beam.sources")
                                        next_type = TableType::BeamSource;
                                else if (name == "beam.targets")
                                        next_type = TableType::BeamTarget;
                                else
                                        return report_error(line_no, "Unknown table '" + name + "'");
                        }
                        else if (line.front() == '[' && line.back() == ']')
                        {
                                std::string name = trim(line.substr(1, line.size() - 2));
                                if (name == "camera")
                                        next_type = TableType::Camera;
                                else if (name == "lighting.ambient")
                                        next_type = TableType::LightingAmbient;
                                else
                                        return report_error(line_no, "Unknown table '" + name + "'");
                        }
                        else
                        {
                                return report_error(line_no, "Invalid table declaration");
                        }
                        std::string header = table_header(next_type);
                        int stage = table_stage(next_type);
                        if (stage == 0)
                                return report_error(line_no, "Invalid table order");
                        if (stage < highest_stage)
                                return report_error(line_no, header + " cannot appear after sections from later stages");
                        if (stage > highest_stage + 1)
                        {
                                std::string message;
                                switch (stage)
                                {
                                case 2:
                                        message = "Lighting section must come after camera section";
                                        break;
                                case 3:
                                        message = "Objects section must come after lighting section";
                                        break;
                                case 4:
                                        message = "Beam section must come after objects section";
                                        break;
                                default:
                                        message = "Sections must follow camera -> lighting -> objects -> beam order";
                                        break;
                                }
                                return report_error(line_no, message);
                        }
                        switch (next_type)
                        {
                        case TableType::Camera:
                                if (camera_declared)
                                        return report_error(line_no, "Multiple " + header + " sections are not allowed");
                                camera_declared = true;
                                break;
                        case TableType::LightingAmbient:
                                if (ambient_declared)
                                        return report_error(line_no, "Multiple " + header + " sections are not allowed");
                                ambient_declared = true;
                                break;
                        case TableType::LightingLightSource:
                                if (!ambient_declared)
                                        return report_error(
                                                line_no, header + " must come after " + table_header(TableType::LightingAmbient));
                                break;
                        default:
                                break;
                        }
                        highest_stage = std::max(highest_stage, stage);
                        current.type = next_type;
                        current.header_line = line_no;
                        current.values.clear();
                        continue;
                }
                size_t eq = line.find('=');
                if (eq == std::string::npos)
                        return report_error(line_no, "Expected key/value pair");
                if (current.type == TableType::None)
                        return report_error(line_no, "Value outside of any table");
                std::string key = trim(line.substr(0, eq));
                std::string value = trim(line.substr(eq + 1));
                if (key.empty() || value.empty())
                        return report_error(line_no, "Invalid key/value pair");
                if (!current.values.emplace(key, std::make_pair(value, line_no)).second)
                        return report_error(line_no, "Duplicate key '" + key + "' in " + table_header(current.type));
        }
        if (!finalize_table(current))
                return false;
        if (!camera_seen)
                return report_error(line_no ? line_no : 1, "Camera section is required");
        if (!ambient_seen)
                return report_error(line_no ? line_no : 1, "Lighting ambient section is required");
        if (cam_dir.length_squared() == 0.0)
                return report_error(line_no ? line_no : 1, "Camera look direction cannot be zero");

        outCamera = Camera(cam_pos, cam_pos + cam_dir.normalized(), fov,
                           double(width) / double(height));
        return true;
}

const std::vector<Material> &Parser::get_materials() { return materials; }

