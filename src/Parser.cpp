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
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace
{

inline void eat_ws(std::string_view &s)
{
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
                s.remove_prefix(1);
}

inline bool to_double(std::string_view sv, double &out)
{
#if defined(__GLIBCXX__) && (__GLIBCXX__ < 20220225)
        char buf[128];
        size_t n = std::min(sv.size(), sizeof(buf) - 1);
        std::memcpy(buf, sv.data(), n);
        buf[n] = 0;
        char *end = nullptr;
        out = std::strtod(buf, &end);
        if (end == buf || *end != '\0')
                return false;
        return true;
#else
        const char *b = sv.data();
        const char *e = sv.data() + sv.size();
        auto res = std::from_chars(b, e, out);
        return res.ec == std::errc{} && res.ptr == e;
#endif
}

inline bool to_int(std::string_view sv, int &out)
{
        const char *b = sv.data();
        const char *e = sv.data() + sv.size();
        auto res = std::from_chars(b, e, out);
        return res.ec == std::errc{} && res.ptr == e;
}

inline Vec3 rgb_to_unit(int r, int g, int b)
{
        return Vec3(r / 255.0, g / 255.0, b / 255.0);
}

inline double alpha_to_unit(int a) { return a / 255.0; }

inline std::string_view trim(std::string_view sv)
{
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
                sv.remove_prefix(1);
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))
                sv.remove_suffix(1);
        return sv;
}

std::string strip_comment(const std::string &line)
{
        std::string result;
        result.reserve(line.size());
        bool in_string = false;
        char prev = '\0';
        for (char ch : line)
        {
                if (ch == '"' && prev != '\\')
                        in_string = !in_string;
                if (ch == '#' && !in_string)
                        break;
                result.push_back(ch);
                prev = ch;
        }
        return result;
}

bool parse_array(std::string_view sv, std::vector<std::string_view> &out)
{
        out.clear();
        sv = trim(sv);
        if (sv.size() < 2 || sv.front() != '[')
                return false;
        sv.remove_prefix(1);
        size_t start = 0;
        bool in_string = false;
        for (size_t i = 0; i < sv.size(); ++i)
        {
                char ch = sv[i];
                if (ch == '"')
                        in_string = !in_string;
                if (!in_string && ch == ',')
                {
                        std::string_view part = trim(sv.substr(start, i - start));
                        if (part.empty())
                                return false;
                        out.push_back(part);
                        start = i + 1;
                }
                else if (!in_string && ch == ']')
                {
                        std::string_view part = trim(sv.substr(start, i - start));
                        if (!part.empty())
                                out.push_back(part);
                        else if (i - start > 0 && !out.empty())
                                return false;
                        std::string_view rest = trim(sv.substr(i + 1));
                        return rest.empty();
                }
        }
        return false;
}

bool parse_vec3(std::string_view sv, Vec3 &out)
{
        std::vector<std::string_view> parts;
        if (!parse_array(sv, parts) || parts.size() != 3)
                return false;
        double x = 0, y = 0, z = 0;
        if (!to_double(parts[0], x) || !to_double(parts[1], y) || !to_double(parts[2], z))
                return false;
        out = Vec3(x, y, z);
        return true;
}

struct ColorDef
{
        int r = 255;
        int g = 255;
        int b = 255;
        int a = 255;
        bool has_alpha = false;
};

bool parse_color(std::string_view sv, ColorDef &out, bool allow_alpha)
{
        std::vector<std::string_view> parts;
        if (!parse_array(sv, parts))
                return false;
        if (parts.size() < 3 || parts.size() > (allow_alpha ? 4u : 3u))
                return false;
        int r = 0, g = 0, b = 0, a = 255;
        if (!to_int(parts[0], r) || !to_int(parts[1], g) || !to_int(parts[2], b))
                return false;
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
                return false;
        if (parts.size() == 4)
        {
                if (!allow_alpha)
                        return false;
                if (!to_int(parts[3], a) || a < 0 || a > 255)
                        return false;
                out.has_alpha = true;
        }
        else
        {
                out.has_alpha = false;
        }
        out.r = r;
        out.g = g;
        out.b = b;
        out.a = a;
        return true;
}

bool parse_bool(std::string_view sv, bool &out)
{
        sv = trim(sv);
        if (sv == "true" || sv == "True" || sv == "TRUE")
        {
                out = true;
                return true;
        }
        if (sv == "false" || sv == "False" || sv == "FALSE")
        {
                out = false;
                return true;
        }
        return false;
}

bool parse_string(std::string_view sv, std::string &out)
{
        sv = trim(sv);
        if (sv.size() < 2 || sv.front() != '"' || sv.back() != '"')
                return false;
        out.assign(sv.substr(1, sv.size() - 2));
        return true;
}

std::string to_lower(std::string_view sv)
{
        std::string res;
        res.reserve(sv.size());
        for (char ch : sv)
                res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        return res;
}

enum class Stage
{
        Camera,
        Lighting,
        Objects,
        Beam
};

enum class Section
{
        None,
        Camera,
        LightingAmbient,
        LightingLight,
        Object,
        BeamSource,
        BeamTarget,
        BeamRoot
};

struct CameraData
{
        bool seen = false;
        bool id_set = false;
        bool position_set = false;
        bool lookdir_set = false;
        bool fov_set = false;
        std::string id;
        Vec3 position{0, 0, -10};
        Vec3 lookdir{0, 0, 1};
        double fov = 60.0;
};

struct AmbientData
{
        bool seen = false;
        bool intensity_set = false;
        bool color_set = false;
        double intensity = 0.0;
        ColorDef color;
};

struct LightData
{
        bool position_set = false;
        bool intensity_set = false;
        bool color_set = false;
        Vec3 position{0, 0, 0};
        double intensity = 1.0;
        ColorDef color;
};

struct ObjectData
{
        bool id_set = false;
        bool type_set = false;
        bool position_set = false;
        bool dir_set = false;
        bool color_set = false;
        bool reflective_set = false;
        bool rotatable_set = false;
        bool movable_set = false;
        bool scorable_set = false;
        bool transparent_set = false;
        bool radius_set = false;
        bool height_set = false;
        bool width_set = false;
        bool length_set = false;
        std::string id;
        std::string type;
        Vec3 position{0, 0, 0};
        Vec3 dir{0, 1, 0};
        ColorDef color;
        double radius = 0.0;
        double height = 0.0;
        double width = 0.0;
        double length = 0.0;
        bool reflective = false;
        bool rotatable = false;
        bool movable = false;
        bool scorable = false;
        bool transparent = false;
};

struct BeamSourceData
{
        bool id_set = false;
        bool intensity_set = false;
        bool position_set = false;
        bool dir_set = false;
        bool color_set = false;
        bool radius_set = false;
        bool length_set = false;
        bool movable_set = false;
        bool scorable_set = false;
        bool with_laser_set = false;
        std::string id;
        Vec3 position{0, 0, 0};
        Vec3 dir{0, 0, 1};
        ColorDef color;
        double intensity = 1.0;
        double radius = 0.1;
        double length = 1.0;
        bool movable = false;
        bool scorable = true;
        bool with_laser = true;
};

struct BeamTargetData
{
        bool id_set = false;
        bool position_set = false;
        bool color_set = false;
        bool radius_set = false;
        bool movable_set = false;
        bool scorable_set = false;
        std::string id;
        Vec3 position{0, 0, 0};
        ColorDef color;
        double radius = 1.0;
        bool movable = false;
        bool scorable = true;
};

struct SectionState
{
        Section type = Section::None;
        int index = -1;
};

bool ensure(bool condition, size_t line, const std::string &message)
{
        if (!condition)
        {
                std::cerr << "Parser error at line " << line << ": " << message << '\n';
                return false;
        }
        return true;
}

bool ensure_post(bool condition, const std::string &message)
{
        if (!condition)
        {
                std::cerr << "Parser error: " << message << '\n';
                return false;
        }
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
                std::cerr << "Parser error: unable to open scene file '" << path << "'\n";
                return false;
        }

        materials.clear();
        outScene.objects.clear();
        outScene.lights.clear();
        outScene.accel.reset();
        outScene.ambient = Ambient(Vec3(1, 1, 1), 0.0);

        CameraData camera;
        AmbientData ambient;
        std::vector<LightData> lights;
        std::vector<ObjectData> objects;
        std::vector<BeamSourceData> beam_sources;
        std::vector<BeamTargetData> beam_targets;
        std::unordered_set<std::string> object_ids;
        std::unordered_set<std::string> beam_ids;

        Stage stage = Stage::Camera;
        SectionState current;
        size_t line_no = 0;

        std::string line_raw;
        while (std::getline(in, line_raw))
        {
                ++line_no;
                std::string cleaned = strip_comment(line_raw);
                std::string_view line = trim(cleaned);
                if (line.empty())
                        continue;

                if (line.front() == '[')
                {
                        bool is_array = (line.size() >= 2 && line[1] == '[');
                        std::string_view body;
                        if (is_array)
                        {
                                if (line.size() < 4 || line.substr(line.size() - 2) != "]]"
                                    )
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": malformed section header" << '\n';
                                        return false;
                                }
                                body = trim(line.substr(2, line.size() - 4));
                        }
                        else
                        {
                                if (line.back() != ']')
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": malformed section header" << '\n';
                                        return false;
                                }
                                body = trim(line.substr(1, line.size() - 2));
                        }

                        std::string section_name = std::string(body);
                        std::string lower = to_lower(section_name);

                        current = SectionState{};

                        if (!is_array && lower == "camera")
                        {
                                if (!ensure(stage == Stage::Camera, line_no,
                                            "[camera] must be the first section"))
                                        return false;
                                if (camera.seen)
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": duplicate [camera] section" << '\n';
                                        return false;
                                }
                                camera.seen = true;
                                current.type = Section::Camera;
                                stage = Stage::Lighting;
                                continue;
                        }

                        if (lower == "lighting.ambient")
                        {
                                if (!ensure(stage == Stage::Lighting, line_no,
                                            "lighting section must follow camera"))
                                        return false;
                                if (ambient.seen)
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": duplicate lighting.ambient section" << '\n';
                                        return false;
                                }
                                ambient.seen = true;
                                current.type = Section::LightingAmbient;
                                continue;
                        }

                        if (lower == "lighting.light_source")
                        {
                                if (stage == Stage::Camera)
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": lighting section must follow camera" << '\n';
                                        return false;
                                }
                                if (stage == Stage::Objects || stage == Stage::Beam)
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": lighting section must precede objects" << '\n';
                                        return false;
                                }
                                stage = Stage::Lighting;
                                lights.emplace_back();
                                current.type = Section::LightingLight;
                                current.index = static_cast<int>(lights.size() - 1);
                                continue;
                        }

                        if (lower == "objects")
                        {
                                if (stage == Stage::Camera)
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": objects section must follow lighting" << '\n';
                                        return false;
                                }
                                if (stage == Stage::Beam)
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": objects section cannot appear after beam section" << '\n';
                                        return false;
                                }
                                stage = Stage::Objects;
                                objects.emplace_back();
                                current.type = Section::Object;
                                current.index = static_cast<int>(objects.size() - 1);
                                continue;
                        }

                        if (!is_array && lower == "beam")
                        {
                                if (stage == Stage::Camera || stage == Stage::Lighting)
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": beam section must follow objects" << '\n';
                                        return false;
                                }
                                stage = Stage::Beam;
                                current.type = Section::BeamRoot;
                                continue;
                        }

                        if (lower == "beam.source" || lower == "beam.sources")
                        {
                                if (stage == Stage::Camera || stage == Stage::Lighting)
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": beam section must follow objects" << '\n';
                                        return false;
                                }
                                stage = Stage::Beam;
                                beam_sources.emplace_back();
                                current.type = Section::BeamSource;
                                current.index = static_cast<int>(beam_sources.size() - 1);
                                continue;
                        }

                        if (lower == "beam.target" || lower == "beam.targets")
                        {
                                if (stage == Stage::Camera || stage == Stage::Lighting)
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": beam section must follow objects" << '\n';
                                        return false;
                                }
                                stage = Stage::Beam;
                                beam_targets.emplace_back();
                                current.type = Section::BeamTarget;
                                current.index = static_cast<int>(beam_targets.size() - 1);
                                continue;
                        }

                        std::cerr << "Parser error at line " << line_no
                                  << ": unknown section '" << section_name << "'" << '\n';
                        return false;
                }

                size_t eq = line.find('=');
                if (eq == std::string_view::npos)
                {
                        std::cerr << "Parser error at line " << line_no
                                  << ": expected key = value" << '\n';
                        return false;
                }
                std::string_view key = trim(line.substr(0, eq));
                std::string_view value = trim(line.substr(eq + 1));
                if (key.empty())
                {
                        std::cerr << "Parser error at line " << line_no
                                  << ": empty key" << '\n';
                        return false;
                }

                switch (current.type)
                {
                case Section::Camera:
                {
                        if (key == "id")
                        {
                                if (camera.id_set || !parse_string(value, camera.id))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid camera id" << '\n';
                                        return false;
                                }
                                camera.id_set = true;
                        }
                        else if (key == "position")
                        {
                                if (camera.position_set || !parse_vec3(value, camera.position))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid camera position" << '\n';
                                        return false;
                                }
                                camera.position_set = true;
                        }
                        else if (key == "lookdir")
                        {
                                if (camera.lookdir_set || !parse_vec3(value, camera.lookdir))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid camera lookdir" << '\n';
                                        return false;
                                }
                                camera.lookdir_set = true;
                        }
                        else if (key == "fov")
                        {
                                if (camera.fov_set || !to_double(value, camera.fov))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid camera fov" << '\n';
                                        return false;
                                }
                                camera.fov_set = true;
                        }
                        else
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": unknown camera key '" << key << "'" << '\n';
                                return false;
                        }
                        break;
                }
                case Section::LightingAmbient:
                {
                        if (key == "intensity")
                        {
                                if (ambient.intensity_set || !to_double(value, ambient.intensity))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid ambient intensity" << '\n';
                                        return false;
                                }
                                ambient.intensity_set = true;
                        }
                        else if (key == "color")
                        {
                                if (ambient.color_set || !parse_color(value, ambient.color, true))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid ambient color" << '\n';
                                        return false;
                                }
                                ambient.color_set = true;
                        }
                        else
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": unknown ambient key '" << key << "'" << '\n';
                                return false;
                        }
                        break;
                }
                case Section::LightingLight:
                {
                        if (current.index < 0 || current.index >= static_cast<int>(lights.size()))
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": internal lighting state error" << '\n';
                                return false;
                        }
                        auto &light = lights[current.index];
                        if (key == "position")
                        {
                                if (light.position_set || !parse_vec3(value, light.position))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid light position" << '\n';
                                        return false;
                                }
                                light.position_set = true;
                        }
                        else if (key == "intensity")
                        {
                                if (light.intensity_set || !to_double(value, light.intensity))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid light intensity" << '\n';
                                        return false;
                                }
                                light.intensity_set = true;
                        }
                        else if (key == "color")
                        {
                                if (light.color_set || !parse_color(value, light.color, true))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid light color" << '\n';
                                        return false;
                                }
                                light.color_set = true;
                        }
                        else
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": unknown light key '" << key << "'" << '\n';
                                return false;
                        }
                        break;
                }
                case Section::Object:
                {
                        if (current.index < 0 || current.index >= static_cast<int>(objects.size()))
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": internal object state error" << '\n';
                                return false;
                        }
                        auto &obj = objects[current.index];
                        if (key == "id")
                        {
                                if (obj.id_set || !parse_string(value, obj.id))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid object id" << '\n';
                                        return false;
                                }
                                obj.id_set = true;
                        }
                        else if (key == "type")
                        {
                                if (obj.type_set || !parse_string(value, obj.type))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid object type" << '\n';
                                        return false;
                                }
                                obj.type_set = true;
                        }
                        else if (key == "position")
                        {
                                if (obj.position_set || !parse_vec3(value, obj.position))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid object position" << '\n';
                                        return false;
                                }
                                obj.position_set = true;
                        }
                        else if (key == "dir")
                        {
                                if (obj.dir_set || !parse_vec3(value, obj.dir))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid object dir" << '\n';
                                        return false;
                                }
                                obj.dir_set = true;
                        }
                        else if (key == "color")
                        {
                                if (obj.color_set || !parse_color(value, obj.color, true))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid object color" << '\n';
                                        return false;
                                }
                                obj.color_set = true;
                        }
                        else if (key == "radius")
                        {
                                if (obj.radius_set || !to_double(value, obj.radius))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid object radius" << '\n';
                                        return false;
                                }
                                obj.radius_set = true;
                        }
                        else if (key == "height")
                        {
                                if (obj.height_set || !to_double(value, obj.height))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid object height" << '\n';
                                        return false;
                                }
                                obj.height_set = true;
                        }
                        else if (key == "width")
                        {
                                if (obj.width_set || !to_double(value, obj.width))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid object width" << '\n';
                                        return false;
                                }
                                obj.width_set = true;
                        }
                        else if (key == "length")
                        {
                                if (obj.length_set || !to_double(value, obj.length))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid object length" << '\n';
                                        return false;
                                }
                                obj.length_set = true;
                        }
                        else if (key == "reflective")
                        {
                                if (obj.reflective_set || !parse_bool(value, obj.reflective))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid reflective flag" << '\n';
                                        return false;
                                }
                                obj.reflective_set = true;
                        }
                        else if (key == "rotatable")
                        {
                                if (obj.rotatable_set || !parse_bool(value, obj.rotatable))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid rotatable flag" << '\n';
                                        return false;
                                }
                                obj.rotatable_set = true;
                        }
                        else if (key == "movable")
                        {
                                if (obj.movable_set || !parse_bool(value, obj.movable))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid movable flag" << '\n';
                                        return false;
                                }
                                obj.movable_set = true;
                        }
                        else if (key == "scorable")
                        {
                                if (obj.scorable_set || !parse_bool(value, obj.scorable))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid scorable flag" << '\n';
                                        return false;
                                }
                                obj.scorable_set = true;
                        }
                        else if (key == "transparent")
                        {
                                if (obj.transparent_set || !parse_bool(value, obj.transparent))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid transparent flag" << '\n';
                                        return false;
                                }
                                obj.transparent_set = true;
                        }
                        else
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": unknown object key '" << key << "'" << '\n';
                                return false;
                        }
                        break;
                }
                case Section::BeamSource:
                {
                        if (current.index < 0 || current.index >= static_cast<int>(beam_sources.size()))
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": internal beam source state error" << '\n';
                                return false;
                        }
                        auto &bs = beam_sources[current.index];
                        if (key == "id")
                        {
                                if (bs.id_set || !parse_string(value, bs.id))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam source id" << '\n';
                                        return false;
                                }
                                bs.id_set = true;
                        }
                        else if (key == "intensity")
                        {
                                if (bs.intensity_set || !to_double(value, bs.intensity))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam intensity" << '\n';
                                        return false;
                                }
                                bs.intensity_set = true;
                        }
                        else if (key == "position")
                        {
                                if (bs.position_set || !parse_vec3(value, bs.position))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam position" << '\n';
                                        return false;
                                }
                                bs.position_set = true;
                        }
                        else if (key == "dir")
                        {
                                if (bs.dir_set || !parse_vec3(value, bs.dir))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam direction" << '\n';
                                        return false;
                                }
                                bs.dir_set = true;
                        }
                        else if (key == "color")
                        {
                                if (bs.color_set || !parse_color(value, bs.color, true))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam color" << '\n';
                                        return false;
                                }
                                bs.color_set = true;
                        }
                        else if (key == "radius")
                        {
                                if (bs.radius_set || !to_double(value, bs.radius))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam radius" << '\n';
                                        return false;
                                }
                                bs.radius_set = true;
                        }
                        else if (key == "length")
                        {
                                if (bs.length_set || !to_double(value, bs.length))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam length" << '\n';
                                        return false;
                                }
                                bs.length_set = true;
                        }
                        else if (key == "movable")
                        {
                                if (bs.movable_set || !parse_bool(value, bs.movable))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam movable flag" << '\n';
                                        return false;
                                }
                                bs.movable_set = true;
                        }
                        else if (key == "scorable")
                        {
                                if (bs.scorable_set || !parse_bool(value, bs.scorable))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam scorable flag" << '\n';
                                        return false;
                                }
                                bs.scorable_set = true;
                        }
                        else if (key == "with_laser")
                        {
                                if (bs.with_laser_set || !parse_bool(value, bs.with_laser))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam with_laser flag" << '\n';
                                        return false;
                                }
                                bs.with_laser_set = true;
                        }
                        else
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": unknown beam source key '" << key << "'" << '\n';
                                return false;
                        }
                        break;
                }
                case Section::BeamTarget:
                {
                        if (current.index < 0 || current.index >= static_cast<int>(beam_targets.size()))
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": internal beam target state error" << '\n';
                                return false;
                        }
                        auto &bt = beam_targets[current.index];
                        if (key == "id")
                        {
                                if (bt.id_set || !parse_string(value, bt.id))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam target id" << '\n';
                                        return false;
                                }
                                bt.id_set = true;
                        }
                        else if (key == "position")
                        {
                                if (bt.position_set || !parse_vec3(value, bt.position))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam target position" << '\n';
                                        return false;
                                }
                                bt.position_set = true;
                        }
                        else if (key == "color")
                        {
                                if (bt.color_set || !parse_color(value, bt.color, true))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam target color" << '\n';
                                        return false;
                                }
                                bt.color_set = true;
                        }
                        else if (key == "radius")
                        {
                                if (bt.radius_set || !to_double(value, bt.radius))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam target radius" << '\n';
                                        return false;
                                }
                                bt.radius_set = true;
                        }
                        else if (key == "movable")
                        {
                                if (bt.movable_set || !parse_bool(value, bt.movable))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam target movable flag" << '\n';
                                        return false;
                                }
                                bt.movable_set = true;
                        }
                        else if (key == "scorable")
                        {
                                if (bt.scorable_set || !parse_bool(value, bt.scorable))
                                {
                                        std::cerr << "Parser error at line " << line_no
                                                  << ": invalid beam target scorable flag" << '\n';
                                        return false;
                                }
                                bt.scorable_set = true;
                        }
                        else
                        {
                                std::cerr << "Parser error at line " << line_no
                                          << ": unknown beam target key '" << key << "'" << '\n';
                                return false;
                        }
                        break;
                }
                case Section::BeamRoot:
                        std::cerr << "Parser error at line " << line_no
                                  << ": unexpected key in [beam] section" << '\n';
                        return false;
                case Section::None:
                        std::cerr << "Parser error at line " << line_no
                                  << ": key-value pair outside of a section" << '\n';
                        return false;
                }
        }

        if (!ensure_post(camera.seen, "camera section is missing"))
                return false;
        if (!ensure_post(camera.id_set, "camera id is missing"))
                return false;
        if (!ensure_post(camera.position_set, "camera position is missing"))
                return false;
        if (!ensure_post(camera.lookdir_set, "camera lookdir is missing"))
                return false;
        if (!ensure_post(camera.fov_set, "camera fov is missing"))
                return false;
        double dir_len = camera.lookdir.length();
        if (!ensure_post(dir_len > 1e-8, "camera lookdir cannot be zero"))
                return false;
        if (!ensure_post(std::fabs(dir_len - 1.0) < 1e-4,
                         "camera lookdir must be normalized"))
                return false;
        camera.lookdir = camera.lookdir.normalized();
        if (!ensure_post(camera.fov > 0.0 && camera.fov < 180.0,
                         "camera fov must be between 0 and 180 degrees"))
                return false;

        if (ambient.seen)
        {
                if (!ensure_post(ambient.intensity_set, "ambient intensity is missing"))
                        return false;
                if (!ensure_post(ambient.color_set, "ambient color is missing"))
                        return false;
                if (!ensure_post(ambient.color.has_alpha,
                                 "ambient color must include alpha component"))
                        return false;
                if (!ensure_post(ambient.intensity >= 0.0 && ambient.intensity <= 1.0,
                                 "ambient intensity must be within [0, 1]"))
                        return false;
        }

        for (size_t i = 0; i < lights.size(); ++i)
        {
                const auto &light = lights[i];
                if (!ensure_post(light.position_set,
                                 "light source " + std::to_string(i + 1) +
                                         " is missing position"))
                        return false;
                if (!ensure_post(light.intensity_set,
                                 "light source " + std::to_string(i + 1) +
                                         " is missing intensity"))
                        return false;
                if (!ensure_post(light.color_set,
                                 "light source " + std::to_string(i + 1) +
                                         " is missing color"))
                        return false;
                if (!ensure_post(light.color.has_alpha,
                                 "light source color must include alpha component"))
                        return false;
                if (!ensure_post(light.intensity >= 0.0,
                                 "light intensity must be non-negative"))
                        return false;
        }

        for (size_t i = 0; i < objects.size(); ++i)
        {
                auto &obj = objects[i];
                if (!ensure_post(obj.id_set,
                                 "object " + std::to_string(i + 1) + " is missing id"))
                        return false;
                if (!object_ids.insert(obj.id).second)
                {
                        std::cerr << "Parser error: duplicate object id '" << obj.id << "'\n";
                        return false;
                }
                if (!ensure_post(obj.type_set,
                                 "object " + std::to_string(i + 1) + " is missing type"))
                        return false;
                if (!ensure_post(obj.position_set,
                                 "object '" + obj.id + "' is missing position"))
                        return false;
                if (!ensure_post(obj.color_set,
                                 "object '" + obj.id + "' is missing color"))
                        return false;
                if (!ensure_post(obj.reflective_set,
                                 "object '" + obj.id + "' is missing reflective flag"))
                        return false;
                if (!ensure_post(obj.rotatable_set,
                                 "object '" + obj.id + "' is missing rotatable flag"))
                        return false;
                if (!ensure_post(obj.movable_set,
                                 "object '" + obj.id + "' is missing movable flag"))
                        return false;
                if (!ensure_post(obj.scorable_set,
                                 "object '" + obj.id + "' is missing scorable flag"))
                        return false;
                if (!ensure_post(obj.transparent_set,
                                 "object '" + obj.id + "' is missing transparent flag"))
                        return false;

                std::string type_lower = to_lower(obj.type);
                if (type_lower == "plane")
                {
                        if (!ensure_post(obj.dir_set,
                                         "plane '" + obj.id + "' is missing dir"))
                                return false;
                        double dir_len = obj.dir.length();
                        if (!ensure_post(dir_len > 1e-8,
                                         "plane '" + obj.id + "' has zero normal"))
                                return false;
                }
                else if (type_lower == "sphere")
                {
                        if (!ensure_post(obj.radius_set,
                                         "sphere '" + obj.id + "' is missing radius"))
                                return false;
                        if (!ensure_post(obj.radius > 0.0,
                                         "sphere '" + obj.id + "' must have positive radius"))
                                return false;
                }
                else if (type_lower == "cylinder")
                {
                        if (!ensure_post(obj.dir_set,
                                         "cylinder '" + obj.id + "' is missing dir"))
                                return false;
                        if (!ensure_post(obj.radius_set,
                                         "cylinder '" + obj.id + "' is missing radius"))
                                return false;
                        if (!ensure_post(obj.height_set,
                                         "cylinder '" + obj.id + "' is missing height"))
                                return false;
                        if (!ensure_post(obj.radius > 0.0,
                                         "cylinder '" + obj.id + "' must have positive radius"))
                                return false;
                        if (!ensure_post(obj.height > 0.0,
                                         "cylinder '" + obj.id + "' must have positive height"))
                                return false;
                        if (!ensure_post(obj.dir.length() > 1e-8,
                                         "cylinder '" + obj.id + "' has zero axis"))
                                return false;
                }
                else if (type_lower == "cone")
                {
                        if (!ensure_post(obj.dir_set,
                                         "cone '" + obj.id + "' is missing dir"))
                                return false;
                        if (!ensure_post(obj.radius_set,
                                         "cone '" + obj.id + "' is missing radius"))
                                return false;
                        if (!ensure_post(obj.height_set,
                                         "cone '" + obj.id + "' is missing height"))
                                return false;
                        if (!ensure_post(obj.radius > 0.0,
                                         "cone '" + obj.id + "' must have positive radius"))
                                return false;
                        if (!ensure_post(obj.height > 0.0,
                                         "cone '" + obj.id + "' must have positive height"))
                                return false;
                        if (!ensure_post(obj.dir.length() > 1e-8,
                                         "cone '" + obj.id + "' has zero axis"))
                                return false;
                }
                else if (type_lower == "cube" || type_lower == "box")
                {
                        if (!ensure_post(obj.dir_set,
                                         "box '" + obj.id + "' is missing dir"))
                                return false;
                        if (!ensure_post(obj.width_set,
                                         "box '" + obj.id + "' is missing width"))
                                return false;
                        if (!ensure_post(obj.height_set,
                                         "box '" + obj.id + "' is missing height"))
                                return false;
                        if (!ensure_post(obj.length_set,
                                         "box '" + obj.id + "' is missing length"))
                                return false;
                        if (!ensure_post(obj.width > 0.0 && obj.height > 0.0 && obj.length > 0.0,
                                         "box '" + obj.id + "' dimensions must be positive"))
                                return false;
                        if (!ensure_post(obj.dir.length() > 1e-8,
                                         "box '" + obj.id + "' has zero orientation"))
                                return false;
                }
                else
                {
                        std::cerr << "Parser error: unknown object type '" << obj.type << "'\n";
                        return false;
                }

                int expected_alpha = obj.transparent ? 125 : 255;
                if (obj.color.has_alpha)
                {
                                if (!ensure_post(obj.color.a == expected_alpha,
                                                 "object '" + obj.id +
                                                         "' alpha must match transparency flag"))
                                        return false;
                }
                else
                {
                        obj.color.a = expected_alpha;
                }
        }

        for (size_t i = 0; i < beam_sources.size(); ++i)
        {
                auto &bs = beam_sources[i];
                if (bs.id_set)
                {
                        if (!beam_ids.insert(bs.id).second)
                        {
                                std::cerr << "Parser error: duplicate beam id '" << bs.id << "'\n";
                                return false;
                        }
                }
                if (!ensure_post(bs.intensity_set,
                                 "beam source " + std::to_string(i + 1) +
                                         " is missing intensity"))
                        return false;
                if (!ensure_post(bs.position_set,
                                 "beam source " + std::to_string(i + 1) +
                                         " is missing position"))
                        return false;
                if (!ensure_post(bs.dir_set,
                                 "beam source " + std::to_string(i + 1) + " is missing dir"))
                        return false;
                if (!ensure_post(bs.color_set,
                                 "beam source " + std::to_string(i + 1) +
                                         " is missing color"))
                        return false;
                if (!ensure_post(bs.radius_set,
                                 "beam source " + std::to_string(i + 1) +
                                         " is missing radius"))
                        return false;
                if (!ensure_post(bs.length_set,
                                 "beam source " + std::to_string(i + 1) +
                                         " is missing length"))
                        return false;
                if (!ensure_post(bs.movable_set,
                                 "beam source " + std::to_string(i + 1) +
                                         " is missing movable flag"))
                        return false;
                if (!ensure_post(bs.scorable_set,
                                 "beam source " + std::to_string(i + 1) +
                                         " is missing scorable flag"))
                        return false;
                if (!bs.with_laser_set)
                        bs.with_laser = true;
                if (!ensure_post(bs.intensity >= 0.0,
                                 "beam intensity must be non-negative"))
                        return false;
                if (!ensure_post(bs.radius > 0.0,
                                 "beam radius must be positive"))
                        return false;
                if (!ensure_post(bs.length > 0.0,
                                 "beam length must be positive"))
                        return false;
                double dir_len = bs.dir.length();
                if (!ensure_post(dir_len > 1e-8,
                                 "beam direction cannot be zero"))
                        return false;
                if (!ensure_post(bs.color.has_alpha,
                                 "beam color must include alpha component"))
                        return false;
        }

        for (size_t i = 0; i < beam_targets.size(); ++i)
        {
                auto &bt = beam_targets[i];
                if (bt.id_set)
                {
                        if (!beam_ids.insert(bt.id).second)
                        {
                                std::cerr << "Parser error: duplicate beam id '" << bt.id << "'\n";
                                return false;
                        }
                }
                if (!ensure_post(bt.position_set,
                                 "beam target " + std::to_string(i + 1) +
                                         " is missing position"))
                        return false;
                if (!ensure_post(bt.color_set,
                                 "beam target " + std::to_string(i + 1) +
                                         " is missing color"))
                        return false;
                if (!ensure_post(bt.radius_set,
                                 "beam target " + std::to_string(i + 1) +
                                         " is missing radius"))
                        return false;
                if (!ensure_post(bt.movable_set,
                                 "beam target " + std::to_string(i + 1) +
                                         " is missing movable flag"))
                        return false;
                if (!ensure_post(bt.scorable_set,
                                 "beam target " + std::to_string(i + 1) +
                                         " is missing scorable flag"))
                        return false;
                if (!ensure_post(bt.color.has_alpha,
                                 "beam target color must include alpha component"))
                        return false;
                if (!ensure_post(bt.radius > 0.0,
                                 "beam target radius must be positive"))
                        return false;
        }

        if (ambient.seen)
        {
                outScene.ambient = Ambient(rgb_to_unit(ambient.color.r, ambient.color.g,
                                                       ambient.color.b),
                                           ambient.intensity);
        }
        else
        {
                outScene.ambient = Ambient(Vec3(1, 1, 1), 0.0);
        }

        for (const auto &light : lights)
        {
                outScene.lights.emplace_back(light.position,
                                              rgb_to_unit(light.color.r, light.color.g,
                                                          light.color.b),
                                              light.intensity);
        }

        int oid = 0;
        int mid = 0;

        for (const auto &obj : objects)
        {
                std::string type_lower = to_lower(obj.type);
                materials.emplace_back();
                Material &mat = materials.back();
                mat.base_color = rgb_to_unit(obj.color.r, obj.color.g, obj.color.b);
                mat.color = mat.base_color;
                mat.alpha = alpha_to_unit(obj.color.a);
                mat.mirror = obj.reflective;
                HittablePtr created;
                if (type_lower == "plane")
                {
                        auto plane = std::make_shared<Plane>(obj.position, obj.dir, oid++, mid);
                        plane->movable = obj.movable;
                        plane->scorable = obj.scorable;
                        created = plane;
                }
                else if (type_lower == "sphere")
                {
                        auto sphere = std::make_shared<Sphere>(obj.position, obj.radius, oid++, mid);
                        sphere->movable = obj.movable;
                        sphere->scorable = obj.scorable;
                        created = sphere;
                }
                else if (type_lower == "cylinder")
                {
                        auto cylinder = std::make_shared<Cylinder>(obj.position, obj.dir, obj.radius,
                                                                   obj.height, oid++, mid);
                        cylinder->movable = obj.movable;
                        cylinder->scorable = obj.scorable;
                        created = cylinder;
                }
                else if (type_lower == "cone")
                {
                        auto cone = std::make_shared<Cone>(obj.position, obj.dir, obj.radius,
                                                           obj.height, oid++, mid);
                        cone->movable = obj.movable;
                        cone->scorable = obj.scorable;
                        created = cone;
                }
                else if (type_lower == "cube" || type_lower == "box")
                {
                        auto cube = std::make_shared<Cube>(obj.position, obj.dir, obj.length,
                                                           obj.width, obj.height, oid++, mid);
                        cube->movable = obj.movable;
                        cube->scorable = obj.scorable;
                        created = cube;
                }
                if (!created)
                {
                        std::cerr << "Parser error: failed to instantiate object of type '"
                                  << obj.type << "'\n";
                        return false;
                }
                outScene.objects.push_back(created);
                ++mid;
        }

        for (const auto &bs : beam_sources)
        {
                materials.emplace_back();
                Material &beam_mat = materials.back();
                Vec3 unit = rgb_to_unit(bs.color.r, bs.color.g, bs.color.b);
                beam_mat.color = unit;
                beam_mat.base_color = unit;
                beam_mat.alpha = alpha_to_unit(bs.color.a);
                beam_mat.random_alpha = true;
                int beam_material = mid++;

                materials.emplace_back();
                materials.back().color = Vec3(1.0, 1.0, 1.0);
                materials.back().base_color = materials.back().color;
                materials.back().alpha = 0.67;
                int big_mat = mid++;

                materials.emplace_back();
                materials.back().color = (Vec3(1.0, 1.0, 1.0) + unit) * 0.5;
                materials.back().base_color = materials.back().color;
                materials.back().alpha = 0.33;
                int mid_mat = mid++;

                materials.emplace_back();
                materials.back().color = unit;
                materials.back().base_color = unit;
                materials.back().alpha = 1.0;
                int small_mat = mid++;

                Vec3 dir_norm = bs.dir.normalized();

                auto beam = std::make_shared<Beam>(bs.position, dir_norm, bs.radius, bs.length,
                                                   bs.intensity, oid, beam_material, big_mat,
                                                   mid_mat, small_mat, bs.with_laser, unit);
                bool scorable = bs.scorable;
                bool movable = bs.movable;
                beam->source->movable = movable;
                beam->source->scorable = scorable;
                beam->source->mid.scorable = scorable;
                beam->source->inner.scorable = scorable;
                if (beam->laser)
                        beam->laser->scorable = scorable;
                if (bs.with_laser)
                {
                        oid += 2;
                        outScene.objects.push_back(beam->laser);
                        outScene.objects.push_back(beam->source);
                        const double cone_cos = std::sqrt(1.0 - 0.25 * 0.25);
                        outScene.lights.emplace_back(
                                bs.position, unit, bs.intensity,
                                std::vector<int>{beam->laser->object_id,
                                                 beam->source->object_id,
                                                 beam->source->mid.object_id},
                                beam->source->object_id, dir_norm, cone_cos, bs.length);
                }
                else
                {
                        oid += 1;
                        outScene.objects.push_back(beam->source);
                        const double cone_cos = std::sqrt(1.0 - 0.25 * 0.25);
                        outScene.lights.emplace_back(
                                bs.position, unit, bs.intensity,
                                std::vector<int>{beam->source->object_id,
                                                 beam->source->mid.object_id},
                                beam->source->object_id, dir_norm, cone_cos, bs.length);
                }
        }

        for (const auto &bt : beam_targets)
        {
                materials.emplace_back();
                materials.back().color = Vec3(0.0, 0.0, 0.0);
                materials.back().base_color = materials.back().color;
                materials.back().alpha = 0.33;
                int big_mat = mid++;

                materials.emplace_back();
                Vec3 inner_mid = rgb_to_unit(bt.color.r, bt.color.g, bt.color.b) * 0.5;
                materials.back().color = inner_mid;
                materials.back().base_color = inner_mid;
                materials.back().alpha = 0.67;
                int mid_mat = mid++;

                materials.emplace_back();
                Vec3 inner_color = rgb_to_unit(bt.color.r, bt.color.g, bt.color.b);
                materials.back().color = inner_color;
                materials.back().base_color = inner_color;
                materials.back().alpha = 1.0;
                int small_mat = mid++;

                auto target = std::make_shared<BeamTarget>(bt.position, bt.radius, oid++,
                                                           big_mat, mid_mat, small_mat);
                target->movable = bt.movable;
                target->scorable = bt.scorable;
                target->mid.scorable = target->scorable;
                target->inner.scorable = target->scorable;
                outScene.objects.push_back(target);
        }

        outCamera = Camera(camera.position, camera.position + camera.lookdir, camera.fov,
                           double(width) / double(height));
        return true;
}

const std::vector<Material> &Parser::get_materials() { return materials; }
