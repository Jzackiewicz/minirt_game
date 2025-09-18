#include "MapSaver.hpp"

#include "BeamTarget.hpp"
#include "Cone.hpp"
#include "Cube.hpp"
#include "Cylinder.hpp"
#include "Laser.hpp"
#include "Plane.hpp"
#include "Sphere.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_set>

namespace
{
std::string format_double(double value)
{
        double rounded = std::round(value * 100000.0) / 100000.0;
        if (std::fabs(rounded) < 1e-6)
                rounded = 0.0;
        double integral = std::round(rounded);
        if (std::fabs(rounded - integral) < 1e-5)
                return std::to_string(static_cast<long long>(integral));
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(5) << rounded;
        return oss.str();
}

std::string vec_to_str(const Vec3 &v)
{
        return format_double(v.x) + ',' + format_double(v.y) + ',' + format_double(v.z);
}

std::string rgba_to_str(const Vec3 &color, double alpha)
{
        auto clamp01 = [](double v) { return std::clamp(v, 0.0, 1.0); };
        int r = static_cast<int>(std::round(clamp01(color.x) * 255.0));
        int g = static_cast<int>(std::round(clamp01(color.y) * 255.0));
        int b = static_cast<int>(std::round(clamp01(color.z) * 255.0));
        int a = static_cast<int>(std::round(clamp01(alpha) * 255.0));
        std::ostringstream oss;
        oss << r << ',' << g << ',' << b << ',' << a;
        return oss.str();
}

std::string nearest_base16_color(const Vec3 &color)
{
        struct NamedColor
        {
                const char *name;
                double r;
                double g;
                double b;
        };

        static const std::array<NamedColor, 16> base16 = {
                NamedColor{"black", 0.0, 0.0, 0.0},
                NamedColor{"white", 255.0, 255.0, 255.0},
                NamedColor{"gray", 128.0, 128.0, 128.0},
                NamedColor{"silver", 192.0, 192.0, 192.0},
                NamedColor{"maroon", 128.0, 0.0, 0.0},
                NamedColor{"red", 255.0, 0.0, 0.0},
                NamedColor{"purple", 128.0, 0.0, 128.0},
                NamedColor{"fushsia", 255.0, 0.0, 255.0},
                NamedColor{"green", 0.0, 128.0, 0.0},
                NamedColor{"lime", 0.0, 255.0, 0.0},
                NamedColor{"olive", 128.0, 128.0, 0.0},
                NamedColor{"yellow", 255.0, 255.0, 0.0},
                NamedColor{"navy", 0.0, 0.0, 128.0},
                NamedColor{"blue", 0.0, 0.0, 255.0},
                NamedColor{"teal", 0.0, 128.0, 128.0},
                NamedColor{"aqua", 0.0, 255.0, 255.0},
        };

        auto clamp01 = [](double v) { return std::clamp(v, 0.0, 1.0); };
        auto clamp255 = [](double v) { return std::clamp(v, 0.0, 255.0); };

        double max_component = std::max({std::fabs(color.x), std::fabs(color.y),
                                         std::fabs(color.z)});
        double r;
        double g;
        double b;
        if (max_component <= 1.0)
        {
                r = clamp01(color.x) * 255.0;
                g = clamp01(color.y) * 255.0;
                b = clamp01(color.z) * 255.0;
        }
        else
        {
                r = clamp255(color.x);
                g = clamp255(color.y);
                b = clamp255(color.z);
        }

        const NamedColor *closest = &base16.front();
        double best = std::numeric_limits<double>::max();
        for (const auto &named : base16)
        {
                double dr = r - named.r;
                double dg = g - named.g;
                double db = b - named.b;
                double dist = dr * dr + dg * dg + db * db;
                if (dist < best)
                {
                        best = dist;
                        closest = &named;
                }
        }

        return closest->name;
}

std::string object_display_name(const Hittable &obj)
{
        switch (obj.shape_type())
        {
        case ShapeType::Sphere:
                return "Sphere";
        case ShapeType::Cube:
                return "Cube";
        case ShapeType::Cylinder:
                return "Cylinder";
        case ShapeType::Cone:
                return "Cone";
        case ShapeType::Plane:
                return "Plane";
        case ShapeType::Beam:
                return "Beam";
        case ShapeType::BeamTarget:
                return "Beam target";
        default:
                return "Object";
        }
}

bool is_rotatable(const Hittable &obj)
{
        switch (obj.shape_type())
        {
        case ShapeType::Plane:
                return false;
        default:
                return true;
        }
}

std::string describe_shape(const Hittable &obj, const Material &mat)
{
        std::ostringstream oss;
        oss << "#\t" << object_display_name(obj) << " - "
            << nearest_base16_color(mat.base_color);
        if (mat.alpha < 0.999)
                oss << ", transparent";
        oss << ", " << (mat.mirror ? "reflective" : "non-reflective") << ", "
            << (is_rotatable(obj) ? "rotatable" : "non-rotatable") << ", "
            << (obj.movable ? "movable" : "immovable");
        return oss.str();
}

std::string describe_plane(const Plane &pl, const Material &mat)
{
        return describe_shape(pl, mat);
}

std::string describe_beam(const Laser &beam)
{
        std::ostringstream oss;
        bool has_source = !beam.source.expired();
        bool movable = false;
        if (auto src = beam.source.lock())
                movable = src->movable;
        oss << "#\t" << (has_source ? "Beam source" : "Beam") << " - "
            << nearest_base16_color(beam.color) << ", radius "
            << format_double(beam.radius) << ", length "
            << format_double(beam.total_length >= 0.0 ? beam.total_length : beam.length)
            << ", " << (movable ? "movable" : "immovable");
        return oss.str();
}

std::string describe_beam_target(const BeamTarget &target,
                                 const std::vector<Material> &mats)
{
        const Material &inner = mats[target.inner.material_id];
        std::ostringstream oss;
        oss << "#\tBeam target - " << nearest_base16_color(inner.base_color);
        if (inner.alpha < 0.999)
                oss << ", transparent";
        oss << ", radius " << format_double(target.radius) << ", "
            << (target.movable ? "movable" : "immovable");
        return oss.str();
}

void write_shape_line(std::ostream &out, const Hittable &obj,
                      const Material &mat)
{
        std::string reflective_flag = mat.mirror ? "R" : "NR";
        std::string move = obj.movable ? "M" : "IM";
        switch (obj.shape_type())
        {
        case ShapeType::Sphere:
        {
                const auto &sp = static_cast<const Sphere &>(obj);
                out << "sp " << vec_to_str(sp.center) << ' ' << format_double(sp.radius) << ' '
                    << rgba_to_str(mat.base_color, mat.alpha) << ' ' << reflective_flag << ' '
                    << move;
                break;
        }
        case ShapeType::Plane:
        {
                const auto &pl = static_cast<const Plane &>(obj);
                out << "pl " << vec_to_str(pl.point) << ' ' << vec_to_str(pl.normal) << ' '
                    << rgba_to_str(mat.base_color, mat.alpha) << ' ' << reflective_flag << ' '
                    << move;
                break;
        }
        case ShapeType::Cylinder:
        {
                const auto &cy = static_cast<const Cylinder &>(obj);
                out << "cy " << vec_to_str(cy.center) << ' ' << vec_to_str(cy.axis) << ' '
                    << format_double(cy.radius * 2.0) << ' ' << format_double(cy.height) << ' '
                    << rgba_to_str(mat.base_color, mat.alpha) << ' ' << reflective_flag << ' '
                    << move;
                break;
        }
        case ShapeType::Cube:
        {
                const auto &cu = static_cast<const Cube &>(obj);
                out << "cu " << vec_to_str(cu.center) << ' ' << vec_to_str(cu.axis[2]) << ' '
                    << format_double(cu.half.x * 2.0) << ' ' << format_double(cu.half.y * 2.0) << ' '
                    << format_double(cu.half.z * 2.0) << ' '
                    << rgba_to_str(mat.base_color, mat.alpha) << ' ' << reflective_flag << ' '
                    << move;
                break;
        }
        case ShapeType::Cone:
        {
                const auto &co = static_cast<const Cone &>(obj);
                out << "co " << vec_to_str(co.center) << ' ' << vec_to_str(co.axis) << ' '
                    << format_double(co.radius * 2.0) << ' ' << format_double(co.height) << ' '
                    << rgba_to_str(mat.base_color, mat.alpha) << ' ' << reflective_flag << ' '
                    << move;
                break;
        }
        default:
                break;
        }
}

void write_beam_target(std::ostream &out, const BeamTarget &bt,
                       const std::vector<Material> &mats)
{
        std::string move = bt.movable ? "M" : "IM";
        const Material &inner = mats[bt.inner.material_id];
        out << "bt " << vec_to_str(bt.center) << ' '
            << rgba_to_str(inner.base_color, inner.alpha) << ' '
            << format_double(bt.radius) << ' ' << move;
}

void write_beam(std::ostream &out, const Laser &beam)
{
        bool movable = false;
        if (auto src = beam.source.lock())
                movable = src->movable;
        std::string move = movable ? "M" : "IM";
        out << "bm " << format_double(beam.light_intensity) << ' '
            << vec_to_str(beam.path.orig) << ' ' << vec_to_str(beam.path.dir) << ' '
            << rgba_to_str(beam.color, 1.0) << ' ' << format_double(beam.radius) << ' '
            << format_double(beam.total_length >= 0.0 ? beam.total_length : beam.length) << ' '
            << move;
}

} // namespace

bool MapSaver::save(const std::string &path, const Scene &scene,
                                         const Camera &camera,
                                         const std::vector<Material> &materials)
{
        std::ofstream out(path);
        if (!out)
                return false;

        out << "#-----------CAMERA-------------\n";
        out << "C " << vec_to_str(camera.origin) << ' ' << vec_to_str(camera.forward)
            << ' ' << format_double(camera.fov_deg) << "\n\n";

        out << "#-----------LIGHTING-------------\n";
        out << "# Ambient light\n";
        out << "A " << format_double(scene.ambient.intensity) << ' '
            << rgba_to_str(scene.ambient.color, 1.0) << "\n";
        int light_index = 1;
        for (const auto &light : scene.lights)
        {
                if (light.attached_id != -1)
                        continue;
                out << "# Light source " << light_index++ << "\n";
                out << "L " << vec_to_str(light.position) << ' ' << format_double(light.intensity)
                    << ' ' << rgba_to_str(light.color, 1.0) << "\n";
        }
        out << "\n";

        std::unordered_set<int> beam_sources;
        std::vector<std::shared_ptr<Laser>> beams;
        for (const auto &obj : scene.objects)
        {
                if (!obj->is_beam())
                        continue;
                auto laser = std::static_pointer_cast<Laser>(obj);
                if (laser->start > 0.0)
                        continue;
                if (auto src = laser->source.lock())
                        beam_sources.insert(src->object_id);
                beams.push_back(laser);
        }

        out << "#-----------OBJECTS-------------\n";
        bool has_objects = false;
        for (const auto &obj : scene.objects)
        {
                if (obj->is_beam())
                        continue;
                if (beam_sources.count(obj->object_id))
                        continue;
                if (obj->shape_type() == ShapeType::Plane ||
                    obj->shape_type() == ShapeType::BeamTarget)
                        continue;
                if (obj->material_id < 0 ||
                    obj->material_id >= static_cast<int>(materials.size()))
                        continue;
                const Material &mat = materials[obj->material_id];
                out << describe_shape(*obj, mat) << "\n";
                write_shape_line(out, *obj, mat);
                out << "\n";
                has_objects = true;
        }
        if (!has_objects)
                out << "#\t(no objects)\n";
        out << "\n";

        out << "#-----------WALLS-------------\n";
        bool has_walls = false;
        for (const auto &obj : scene.objects)
        {
                if (obj->is_beam())
                        continue;
                if (obj->shape_type() != ShapeType::Plane)
                        continue;
                if (obj->material_id < 0 ||
                    obj->material_id >= static_cast<int>(materials.size()))
                        continue;
                const Material &mat = materials[obj->material_id];
                const auto &pl = static_cast<const Plane &>(*obj);
                out << describe_plane(pl, mat) << "\n";
                write_shape_line(out, *obj, mat);
                out << "\n";
                has_walls = true;
        }
        if (!has_walls)
                out << "#\t(no walls)\n";
        out << "\n";

        out << "#-----------BEAMS-------------\n";
        bool has_beams = false;
        for (const auto &beam : beams)
        {
                out << describe_beam(*beam) << "\n";
                write_beam(out, *beam);
                out << "\n";
                has_beams = true;
        }
        for (const auto &obj : scene.objects)
        {
                if (obj->shape_type() != ShapeType::BeamTarget)
                        continue;
                const auto &bt = static_cast<const BeamTarget &>(*obj);
                out << describe_beam_target(bt, materials) << "\n";
                write_beam_target(out, bt, materials);
                out << "\n";
                has_beams = true;
        }
        if (!has_beams)
                out << "#\t(no beams)\n";

        return true;
}
