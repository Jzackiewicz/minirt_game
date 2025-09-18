#include "MapSaver.hpp"

#include "BeamSource.hpp"
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
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

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

int clamp_byte(double value)
{
        value = std::clamp(value, 0.0, 1.0);
        return static_cast<int>(std::round(value * 255.0));
}

std::string vec_to_array(const Vec3 &v)
{
        std::ostringstream oss;
        oss << '[' << format_double(v.x) << ", " << format_double(v.y) << ", "
            << format_double(v.z) << ']';
        return oss.str();
}

std::string color_rgb_array(const Vec3 &color)
{
        std::ostringstream oss;
        oss << '[' << clamp_byte(color.x) << ", " << clamp_byte(color.y) << ", "
            << clamp_byte(color.z) << ']';
        return oss.str();
}

std::string color_rgba_array(const Vec3 &color, double alpha)
{
        std::ostringstream oss;
        oss << '[' << clamp_byte(color.x) << ", " << clamp_byte(color.y) << ", "
            << clamp_byte(color.z) << ", " << clamp_byte(alpha) << ']';
        return oss.str();
}

std::string bool_to_string(bool value) { return value ? "true" : "false"; }

std::string object_type_string(const Hittable &obj)
{
        switch (obj.shape_type())
        {
        case ShapeType::Plane:
                return "plane";
        case ShapeType::Sphere:
                return "sphere";
        case ShapeType::Cylinder:
                return "cylinder";
        case ShapeType::Cone:
                return "cone";
        case ShapeType::Cube:
                return "box";
        default:
                return "object";
        }
}

std::string make_object_id(const Hittable &obj)
{
        return object_type_string(obj) + '_' + std::to_string(obj.object_id);
}

Vec3 object_position(const Hittable &obj)
{
        switch (obj.shape_type())
        {
        case ShapeType::Plane:
                return static_cast<const Plane &>(obj).point;
        case ShapeType::Sphere:
                return static_cast<const Sphere &>(obj).center;
        case ShapeType::Cylinder:
                return static_cast<const Cylinder &>(obj).center;
        case ShapeType::Cone:
                return static_cast<const Cone &>(obj).center;
        case ShapeType::Cube:
                return static_cast<const Cube &>(obj).center;
        default:
                return Vec3(0, 0, 0);
        }
}

Vec3 object_direction(const Hittable &obj)
{
        switch (obj.shape_type())
        {
        case ShapeType::Plane:
                return static_cast<const Plane &>(obj).normal;
        case ShapeType::Cylinder:
                return static_cast<const Cylinder &>(obj).axis;
        case ShapeType::Cone:
                return static_cast<const Cone &>(obj).axis;
        case ShapeType::Cube:
                return static_cast<const Cube &>(obj).axis[2];
        default:
                return Vec3(0, 1, 0);
        }
}

double object_radius(const Hittable &obj)
{
        switch (obj.shape_type())
        {
        case ShapeType::Sphere:
                return static_cast<const Sphere &>(obj).radius;
        case ShapeType::Cylinder:
                return static_cast<const Cylinder &>(obj).radius;
        case ShapeType::Cone:
                return static_cast<const Cone &>(obj).radius;
        default:
                return 0.0;
        }
}

double object_height(const Hittable &obj)
{
        switch (obj.shape_type())
        {
        case ShapeType::Cylinder:
                return static_cast<const Cylinder &>(obj).height;
        case ShapeType::Cone:
                return static_cast<const Cone &>(obj).height;
        default:
                return 0.0;
        }
}

std::array<double, 3> cube_dimensions(const Cube &cube)
{
        return {cube.half.x * 2.0, cube.half.y * 2.0, cube.half.z * 2.0};
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

void write_object(std::ostream &out, const Hittable &obj, const Material &mat)
{
        std::string type = object_type_string(obj);
        Vec3 position = object_position(obj);
        Vec3 direction = object_direction(obj);
        Vec3 dir_norm = direction.length() > 1e-8 ? direction.normalized() : Vec3(0, 1, 0);
        bool transparent = mat.alpha < 0.999;

        out << "[[objects]]\n";
        out << "id = \"" << make_object_id(obj) << "\"\n";
        out << "type = \"" << type << "\"\n";
        out << "color = " << color_rgb_array(mat.base_color) << "\n";
        out << "position = " << vec_to_array(position) << "\n";
        out << "dir = " << vec_to_array(dir_norm) << "\n";

        if (obj.shape_type() == ShapeType::Sphere)
        {
                out << "radius = " << format_double(object_radius(obj)) << "\n";
        }
        else if (obj.shape_type() == ShapeType::Cylinder)
        {
                out << "radius = " << format_double(object_radius(obj)) << "\n";
                out << "height = " << format_double(object_height(obj)) << "\n";
        }
        else if (obj.shape_type() == ShapeType::Cone)
        {
                out << "radius = " << format_double(object_radius(obj)) << "\n";
                out << "height = " << format_double(object_height(obj)) << "\n";
        }
        else if (obj.shape_type() == ShapeType::Cube)
        {
                const auto &cube = static_cast<const Cube &>(obj);
                auto dims = cube_dimensions(cube);
                out << "width = " << format_double(dims[0]) << "\n";
                out << "height = " << format_double(dims[1]) << "\n";
                out << "length = " << format_double(dims[2]) << "\n";
        }

        out << "reflective = " << bool_to_string(mat.mirror) << "\n";
        out << "rotatable = " << bool_to_string(is_rotatable(obj)) << "\n";
        out << "movable = " << bool_to_string(obj.movable) << "\n";
        out << "scorable = " << bool_to_string(obj.scorable) << "\n";
        out << "transparent = " << bool_to_string(transparent) << "\n\n";
}

struct BeamSourceInfo
{
        std::string id;
        Vec3 position;
        Vec3 direction;
        Vec3 color;
        double intensity = 1.0;
        double radius = 0.1;
        double length = 1.0;
        bool movable = false;
        bool scorable = true;
        bool with_laser = true;
};

struct BeamTargetInfo
{
        std::string id;
        Vec3 position;
        Vec3 color;
        double alpha = 1.0;
        double radius = 1.0;
        bool movable = false;
        bool scorable = true;
};

std::string beam_source_id(const BeamSource &source)
{
        return "beam_source_" + std::to_string(source.object_id);
}

std::string beam_target_id(const BeamTarget &target)
{
        return "beam_target_" + std::to_string(target.object_id);
}

std::string beam_color_array(const Vec3 &color)
{
        std::ostringstream oss;
        oss << '[' << clamp_byte(color.x) << ", " << clamp_byte(color.y) << ", "
            << clamp_byte(color.z) << ", 255]";
        return oss.str();
}

std::string beam_color_array(const Vec3 &color, double alpha)
{
        std::ostringstream oss;
        oss << '[' << clamp_byte(color.x) << ", " << clamp_byte(color.y) << ", "
            << clamp_byte(color.z) << ", " << clamp_byte(alpha) << ']';
        return oss.str();
}

} // namespace

bool MapSaver::save(const std::string &path, const Scene &scene,
                                         const Camera &camera,
                                         const std::vector<Material> &materials)
{
        std::ofstream out(path);
        if (!out)
                return false;

        Vec3 forward = camera.forward.length() > 1e-8 ? camera.forward.normalized()
                                                       : Vec3(0, 0, 1);

        out << "[camera]\n";
        out << "id = \"cam\"\n";
        out << "position = " << vec_to_array(camera.origin) << "\n";
        out << "lookdir = " << vec_to_array(forward) << "\n";
        out << "fov = " << format_double(camera.fov_deg) << "\n\n";

        out << "[lighting.ambient]\n";
        out << "intensity = " << format_double(scene.ambient.intensity) << "\n";
        out << "color = " << color_rgba_array(scene.ambient.color, 1.0) << "\n\n";

        for (const auto &light : scene.lights)
        {
                if (light.attached_id != -1)
                        continue;
                out << "[[lighting.light_source]]\n";
                out << "intensity = " << format_double(light.intensity) << "\n";
                out << "position = " << vec_to_array(light.position) << "\n";
                out << "color = " << color_rgba_array(light.color, 1.0) << "\n\n";
        }

        std::unordered_set<int> skip_ids;
        std::vector<BeamSourceInfo> beam_sources;
        std::vector<BeamTargetInfo> beam_targets;

        for (const auto &obj : scene.objects)
        {
                if (auto laser = std::dynamic_pointer_cast<Laser>(obj))
                {
                        if (laser->start > 0.0)
                                continue;
                        BeamSourceInfo info;
                        info.position = laser->path.orig;
                        info.direction = laser->path.dir;
                        info.color = laser->color;
                        info.intensity = laser->light_intensity;
                        info.radius = laser->radius;
                        info.length = (laser->total_length >= 0.0) ? laser->total_length
                                                                  : laser->length;
                        info.scorable = laser->scorable;
                        info.with_laser = true;
                        if (auto src = laser->source.lock())
                        {
                                info.movable = src->movable;
                                info.id = beam_source_id(*src);
                                skip_ids.insert(src->object_id);
                        }
                        else
                        {
                                info.movable = false;
                                info.id = "beam_source_" + std::to_string(laser->object_id);
                        }
                        skip_ids.insert(laser->object_id);
                        beam_sources.push_back(info);
                        continue;
                }

                if (auto src = std::dynamic_pointer_cast<BeamSource>(obj))
                {
                        skip_ids.insert(src->object_id);
                        if (!src->beam)
                        {
                                BeamSourceInfo info;
                                info.id = beam_source_id(*src);
                                info.position = src->center;
                                info.direction = src->spot_direction();
                                if (src->light)
                                {
                                        info.color = src->light->color;
                                        info.intensity = src->light->intensity;
                                        info.radius = src->light->radius;
                                }
                                else
                                {
                                        info.color = Vec3(1.0, 1.0, 1.0);
                                        info.intensity = 1.0;
                                        info.radius = src->inner.radius;
                                }
                                info.length = 1.0;
                                info.movable = src->movable;
                                info.scorable = src->scorable;
                                info.with_laser = false;
                                beam_sources.push_back(info);
                        }
                        continue;
                }

                if (obj->shape_type() == ShapeType::BeamTarget)
                {
                        auto target = std::static_pointer_cast<BeamTarget>(obj);
                        BeamTargetInfo info;
                        info.id = beam_target_id(*target);
                        info.position = target->center;
                        info.radius = target->radius;
                        info.movable = target->movable;
                        info.scorable = target->scorable;
                        if (target->inner.material_id >= 0 &&
                            target->inner.material_id < static_cast<int>(materials.size()))
                        {
                                const Material &inner = materials[target->inner.material_id];
                                info.color = inner.base_color;
                                info.alpha = inner.alpha;
                        }
                        beam_targets.push_back(info);
                        skip_ids.insert(target->object_id);
                        continue;
                }
        }

        for (const auto &obj : scene.objects)
        {
                if (skip_ids.count(obj->object_id))
                        continue;
                if (obj->material_id < 0 ||
                    obj->material_id >= static_cast<int>(materials.size()))
                        continue;
                const Material &mat = materials[obj->material_id];
                if (obj->is_beam())
                        continue;
                if (obj->shape_type() == ShapeType::BeamTarget)
                        continue;
                write_object(out, *obj, mat);
        }

        out << "[beam]\n";
        if (!beam_sources.empty())
                out << '\n';
        for (const auto &bs : beam_sources)
        {
                out << "[[beam.sources]]\n";
                out << "id = \"" << (bs.id.empty() ? "beam_source" : bs.id) << "\"\n";
                out << "intensity = " << format_double(bs.intensity) << "\n";
                out << "position = " << vec_to_array(bs.position) << "\n";
                out << "dir = " << vec_to_array(bs.direction) << "\n";
                out << "color = " << beam_color_array(bs.color) << "\n";
                out << "radius = " << format_double(bs.radius) << "\n";
                out << "length = " << format_double(bs.length) << "\n";
                out << "movable = " << bool_to_string(bs.movable) << "\n";
                out << "scorable = " << bool_to_string(bs.scorable) << "\n";
                out << "with_laser = " << bool_to_string(bs.with_laser) << "\n\n";
        }

        for (const auto &bt : beam_targets)
        {
                out << "[[beam.targets]]\n";
                out << "id = \"" << (bt.id.empty() ? "beam_target" : bt.id) << "\"\n";
                out << "position = " << vec_to_array(bt.position) << "\n";
                out << "color = " << beam_color_array(bt.color, bt.alpha) << "\n";
                out << "radius = " << format_double(bt.radius) << "\n";
                out << "movable = " << bool_to_string(bt.movable) << "\n";
                out << "scorable = " << bool_to_string(bt.scorable) << "\n\n";
        }

        return true;
}
