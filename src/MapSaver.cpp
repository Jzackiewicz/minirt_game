#include "MapSaver.hpp"

#include "BeamTarget.hpp"
#include "Cone.hpp"
#include "Cube.hpp"
#include "Cylinder.hpp"
#include "Laser.hpp"
#include "Plane.hpp"
#include "Sphere.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <ostream>
#include <sstream>
#include <unordered_set>

namespace
{
struct Rgba
{
        int r;
        int g;
        int b;
        int a;
};

int to_channel(double value)
{
        int channel = static_cast<int>(std::round(value));
        return std::clamp(channel, 0, 255);
}

Rgba to_rgba(const Vec3 &color, double alpha)
{
        return {
                to_channel(color.x * 255.0),
                to_channel(color.y * 255.0),
                to_channel(color.z * 255.0),
                to_channel(alpha * 255.0),
        };
}

std::string format_rgba(const Rgba &rgba)
{
        std::ostringstream oss;
        oss << rgba.r << ',' << rgba.g << ',' << rgba.b << ',' << rgba.a;
        return oss.str();
}

std::string format_vec(const Vec3 &vec)
{
        std::ostringstream oss;
        oss << vec.x << ',' << vec.y << ',' << vec.z;
        return oss.str();
}

std::string describe_object(const std::string &label, const Rgba &rgba, bool mirror,
                                                    bool movable)
{
        std::ostringstream oss;
        oss << "#\t" << label << " - color " << format_rgba(rgba) << ", "
            << (mirror ? "mirror" : "non-mirror") << ", "
            << (movable ? "movable" : "immovable");
        return oss.str();
}

std::string describe_beam(const std::string &label, const Rgba &rgba, bool movable)
{
        std::ostringstream oss;
        oss << "#\t" << label << " - color " << format_rgba(rgba) << ", "
            << (movable ? "movable" : "immovable");
        return oss.str();
}

std::unordered_set<int> collect_beam_sources(const Scene &scene)
{
        std::unordered_set<int> result;
        for (const auto &object : scene.objects)
        {
                if (!object->is_beam())
                        continue;
                auto laser = std::static_pointer_cast<Laser>(object);
                if (auto source = laser->source.lock())
                        result.insert(source->object_id);
        }
        return result;
}

void write_camera(std::ostream &out, const Camera &camera)
{
        out << "#-----------CAMERA-------------\n";
        out << "C " << format_vec(camera.origin) << ' ' << format_vec(camera.forward)
            << ' ' << camera.fov_deg << '\n';
}

void write_lighting(std::ostream &out, const Scene &scene)
{
        out << "#-----------LIGHTING-------------\n";
        out << "# Ambient light\n";
        out << "A " << scene.ambient.intensity << ' '
            << format_rgba(to_rgba(scene.ambient.color, 1.0)) << '\n';

        bool has_light = false;
        for (const auto &light : scene.lights)
        {
                if (light.attached_id != -1)
                        continue;
                out << "# Light source\n";
                out << "L " << format_vec(light.position) << ' ' << light.intensity << ' '
                    << format_rgba(to_rgba(light.color, 1.0)) << '\n';
                has_light = true;
        }
        if (!has_light)
        {
                out << "# Light source\n";
                out << "#\t(none)\n";
        }
}

void write_objects(std::ostream &out, const Scene &scene,
                                   const std::vector<Material> &materials,
                                   const std::unordered_set<int> &beam_sources)
{
        out << "#-----------OBJECTS-------------\n";
        bool has_object = false;

        for (const auto &object : scene.objects)
        {
                if (object->is_beam())
                        continue;
                if (beam_sources.count(object->object_id) != 0)
                        continue;
                if (object->shape_type() == ShapeType::Plane)
                        continue;
                if (object->shape_type() == ShapeType::BeamTarget)
                        continue;

                const Material &material = materials[object->material_id];
                const Rgba rgba = to_rgba(material.base_color, material.alpha);
                const bool mirror = material.mirror;
                const bool movable = object->movable;

                switch (object->shape_type())
                {
                case ShapeType::Sphere:
                {
                        auto sphere = static_cast<const Sphere *>(object.get());
                        out << describe_object("Sphere", rgba, mirror, movable) << '\n';
                        out << "sp " << format_vec(sphere->center) << ' '
                            << sphere->radius << ' ' << format_rgba(rgba) << ' '
                            << (mirror ? "R" : "NR") << ' '
                            << (movable ? "M" : "IM") << '\n';
                        has_object = true;
                        break;
                }
                case ShapeType::Cube:
                {
                        auto cube = static_cast<const Cube *>(object.get());
                        out << describe_object("Cube", rgba, mirror, movable) << '\n';
                        out << "cu " << format_vec(cube->center) << ' '
                            << format_vec(cube->axis[2]) << ' '
                            << (cube->half.x * 2.0) << ' ' << (cube->half.y * 2.0) << ' '
                            << (cube->half.z * 2.0) << ' ' << format_rgba(rgba) << ' '
                            << (mirror ? "R" : "NR") << ' '
                            << (movable ? "M" : "IM") << '\n';
                        has_object = true;
                        break;
                }
                case ShapeType::Cylinder:
                {
                        auto cylinder = static_cast<const Cylinder *>(object.get());
                        out << describe_object("Cylinder", rgba, mirror, movable) << '\n';
                        out << "cy " << format_vec(cylinder->center) << ' '
                            << format_vec(cylinder->axis) << ' '
                            << (cylinder->radius * 2.0) << ' ' << cylinder->height << ' '
                            << format_rgba(rgba) << ' '
                            << (mirror ? "R" : "NR") << ' '
                            << (movable ? "M" : "IM") << '\n';
                        has_object = true;
                        break;
                }
                case ShapeType::Cone:
                {
                        auto cone = static_cast<const Cone *>(object.get());
                        out << describe_object("Cone", rgba, mirror, movable) << '\n';
                        out << "co " << format_vec(cone->center) << ' '
                            << format_vec(cone->axis) << ' ' << (cone->radius * 2.0) << ' '
                            << cone->height << ' ' << format_rgba(rgba) << ' '
                            << (mirror ? "R" : "NR") << ' '
                            << (movable ? "M" : "IM") << '\n';
                        has_object = true;
                        break;
                }
                default:
                        break;
                }
        }

        if (!has_object)
                out << "#\t(no objects)\n";
}

void write_walls(std::ostream &out, const Scene &scene,
                                const std::vector<Material> &materials)
{
        out << "#-----------WALLS-------------\n";
        bool has_wall = false;

        for (const auto &object : scene.objects)
        {
                if (object->shape_type() != ShapeType::Plane)
                        continue;
                const Material &material = materials[object->material_id];
                const Rgba rgba = to_rgba(material.base_color, material.alpha);
                auto plane = static_cast<const Plane *>(object.get());
                out << "pl " << format_vec(plane->point) << ' '
                    << format_vec(plane->normal) << ' ' << format_rgba(rgba) << ' '
                    << (material.mirror ? "R" : "NR") << ' '
                    << (object->movable ? "M" : "IM") << '\n';
                has_wall = true;
        }

        if (!has_wall)
                out << "#\t(no walls)\n";
}

void write_beams(std::ostream &out, const Scene &scene,
                                const std::vector<Material> &materials)
{
        out << "#-----------BEAMS-------------\n";
        bool has_beam = false;

        for (const auto &object : scene.objects)
        {
                if (!object->is_beam())
                        continue;
                auto laser = std::static_pointer_cast<Laser>(object);
                if (laser->start > 0.0)
                        continue;
                const Material &material = materials[laser->material_id];
                const Rgba rgba = to_rgba(material.base_color, material.alpha);
                bool movable = false;
                if (auto source = laser->source.lock())
                        movable = source->movable;

                out << describe_beam("Beam source", rgba, movable) << '\n';
                out << "bm " << laser->light_intensity << ' '
                    << format_vec(laser->path.orig) << ' '
                    << format_vec(laser->path.dir) << ' ' << format_rgba(rgba) << ' '
                    << laser->radius << ' ' << laser->total_length << ' '
                    << (movable ? "M" : "IM") << '\n';
                has_beam = true;
        }

        for (const auto &object : scene.objects)
        {
                if (object->shape_type() != ShapeType::BeamTarget)
                        continue;
                auto target = std::static_pointer_cast<BeamTarget>(object);
                const Material &material = materials[target->material_id];
                const Rgba rgba = to_rgba(material.base_color, material.alpha);
                out << describe_beam("Beam target", rgba, target->movable) << '\n';
                out << "bt " << format_vec(target->center) << ' ' << format_rgba(rgba)
                    << ' ' << target->radius << ' ' << (target->movable ? "M" : "IM")
                    << '\n';
                has_beam = true;
        }

        if (!has_beam)
                out << "#\t(no beams)\n";
}

} // namespace

bool MapSaver::save(const std::string &path, const Scene &scene,
                                        const Camera &camera,
                                        const std::vector<Material> &materials)
{
        std::ofstream out(path);
        if (!out)
                return false;

        const auto beam_sources = collect_beam_sources(scene);

        write_camera(out, camera);
        out << '\n';
        write_lighting(out, scene);
        out << '\n';
        write_objects(out, scene, materials, beam_sources);
        out << '\n';
        write_walls(out, scene, materials);
        out << '\n';
        write_beams(out, scene, materials);

        return static_cast<bool>(out);
}
