#include "MapSaver.hpp"

#include "BeamSource.hpp"
#include "BeamTarget.hpp"
#include "Camera.hpp"
#include "Cone.hpp"
#include "Cube.hpp"
#include "Cylinder.hpp"
#include "Laser.hpp"
#include "Plane.hpp"
#include "Scene.hpp"
#include "Sphere.hpp"
#include "material.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>

namespace
{

std::string format_scalar(double value)
{
        if (std::abs(value) < 1e-9)
                value = 0.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(5) << value;
        std::string s = oss.str();
        auto dot = s.find('.');
        if (dot != std::string::npos)
        {
                while (!s.empty() && s.back() == '0')
                        s.pop_back();
                if (!s.empty() && s.back() == '.')
                        s.pop_back();
        }
        if (s.empty())
                s = "0";
        return s;
}

std::string format_vec(const Vec3 &v)
{
        return format_scalar(v.x) + ',' + format_scalar(v.y) + ',' + format_scalar(v.z);
}

int to_channel(double value)
{
        long rounded = std::lround(value * 255.0);
        rounded = std::clamp(rounded, 0l, 255l);
        return static_cast<int>(rounded);
}

std::string format_rgba(const Vec3 &color, double alpha)
{
        std::ostringstream oss;
        oss << to_channel(color.x) << ',' << to_channel(color.y) << ','
                << to_channel(color.z) << ',' << to_channel(alpha);
        return oss.str();
}

std::string describe_rgba(const Vec3 &color, double alpha)
{
        return format_rgba(color, alpha);
}

std::string mirror_token(const Material &mat)
{
        return mat.mirror ? "R" : "NR";
}

std::string move_token(const Hittable &obj)
{
        return obj.movable ? "M" : "IM";
}

std::string shape_name(ShapeType type)
{
        switch (type)
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

std::string describe_object_comment(const std::string &label, const Hittable &obj,
                                                                        const Material &mat)
{
        std::ostringstream oss;
        oss << label << " (id: " << obj.object_id << ") color="
                << describe_rgba(mat.base_color, mat.alpha)
                << " mirror=" << (mat.mirror ? "yes" : "no")
                << " movable=" << (obj.movable ? "yes" : "no");
        return oss.str();
}

std::unordered_set<int> collect_beam_related_ids(const Scene &scene)
{
        std::unordered_set<int> ids;
        ids.reserve(scene.objects.size());
        for (const auto &obj : scene.objects)
        {
                if (!obj)
                        continue;
                if (auto source = std::dynamic_pointer_cast<BeamSource>(obj))
                {
                        ids.insert(source->object_id);
                        ids.insert(source->mid.object_id);
                        ids.insert(source->inner.object_id);
                        continue;
                }
                if (!obj->is_beam())
                        continue;
                auto laser = std::static_pointer_cast<Laser>(obj);
                if (laser->start > 0.0)
                        continue;
                if (auto src = laser->source.lock())
                {
                        ids.insert(src->object_id);
                        ids.insert(src->mid.object_id);
                        ids.insert(src->inner.object_id);
                }
        }
        return ids;
}

void write_camera_section(std::ostream &out, const Camera &cam)
{
        out << "#-----------CAMERA-------------\n";
        out << "C " << format_vec(cam.origin) << ' ' << format_vec(cam.forward) << ' '
                << format_scalar(cam.fov_deg) << "\n";
}

void write_lighting_section(std::ostream &out, const Scene &scene)
{
        out << "#-----------LIGHTING-------------\n";
        out << "# Ambient light\n";
        out << "A " << format_scalar(scene.ambient.intensity) << ' '
                << format_rgba(scene.ambient.color, 1.0) << "\n";

        int light_index = 0;
        for (const auto &light : scene.lights)
        {
                if (light.attached_id != -1)
                        continue;
                ++light_index;
                out << "# Light source " << light_index << "\n";
                out << "L " << format_vec(light.position) << ' '
                        << format_scalar(light.intensity) << ' '
                        << format_rgba(light.color, 1.0) << "\n";
        }
        if (light_index == 0)
                out << "# No independent light sources\n";
}

void write_objects_section(std::ostream &out, const Scene &scene,
                                                  const std::vector<Material> &materials,
                                                  const std::unordered_set<int> &beam_related)
{
        out << "#-----------OBJECTS-------------\n";
        bool any = false;
        for (const auto &obj : scene.objects)
        {
                if (!obj)
                        continue;
                if (obj->is_beam())
                        continue;
                if (beam_related.count(obj->object_id) != 0)
                        continue;
                if (obj->shape_type() == ShapeType::Plane)
                        continue;
                if (obj->shape_type() == ShapeType::BeamTarget)
                        continue;
                const Material &mat = materials[obj->material_id];
                switch (obj->shape_type())
                {
                case ShapeType::Sphere:
                {
                        auto sp = static_cast<const Sphere *>(obj.get());
                        out << "# " << describe_object_comment(shape_name(obj->shape_type()),
                                                                                 *obj, mat)
                                << "\n";
                        out << "sp " << format_vec(sp->center) << ' '
                                << format_scalar(sp->radius) << ' '
                                << format_rgba(mat.base_color, mat.alpha) << ' '
                                << mirror_token(mat) << ' ' << move_token(*obj) << "\n";
                        any = true;
                        break;
                }
                case ShapeType::Cube:
                {
                        auto cu = static_cast<const Cube *>(obj.get());
                        out << "# " << describe_object_comment(shape_name(obj->shape_type()),
                                                                                 *obj, mat)
                                << "\n";
                        out << "cu " << format_vec(cu->center) << ' '
                                << format_vec(cu->axis[2]) << ' '
                                << format_scalar(cu->half.x * 2.0) << ' '
                                << format_scalar(cu->half.y * 2.0) << ' '
                                << format_scalar(cu->half.z * 2.0) << ' '
                                << format_rgba(mat.base_color, mat.alpha) << ' '
                                << mirror_token(mat) << ' ' << move_token(*obj) << "\n";
                        any = true;
                        break;
                }
                case ShapeType::Cylinder:
                {
                        auto cy = static_cast<const Cylinder *>(obj.get());
                        out << "# " << describe_object_comment(shape_name(obj->shape_type()),
                                                                                 *obj, mat)
                                << "\n";
                        out << "cy " << format_vec(cy->center) << ' '
                                << format_vec(cy->axis) << ' '
                                << format_scalar(cy->radius * 2.0) << ' '
                                << format_scalar(cy->height) << ' '
                                << format_rgba(mat.base_color, mat.alpha) << ' '
                                << mirror_token(mat) << ' ' << move_token(*obj) << "\n";
                        any = true;
                        break;
                }
                case ShapeType::Cone:
                {
                        auto co = static_cast<const Cone *>(obj.get());
                        out << "# " << describe_object_comment(shape_name(obj->shape_type()),
                                                                                 *obj, mat)
                                << "\n";
                        out << "co " << format_vec(co->center) << ' '
                                << format_vec(co->axis) << ' '
                                << format_scalar(co->radius * 2.0) << ' '
                                << format_scalar(co->height) << ' '
                                << format_rgba(mat.base_color, mat.alpha) << ' '
                                << mirror_token(mat) << ' ' << move_token(*obj) << "\n";
                        any = true;
                        break;
                }
                default:
                        break;
                }
        }
        if (!any)
                out << "# No standalone objects\n";
}

void write_walls_section(std::ostream &out, const Scene &scene,
                                                 const std::vector<Material> &materials,
                                                 const std::unordered_set<int> &beam_related)
{
        out << "#-----------WALLS-------------\n";
        bool any = false;
        for (const auto &obj : scene.objects)
        {
                if (!obj)
                        continue;
                if (obj->shape_type() != ShapeType::Plane)
                        continue;
                if (beam_related.count(obj->object_id) != 0)
                        continue;
                const Material &mat = materials[obj->material_id];
                auto pl = static_cast<const Plane *>(obj.get());
                out << "# " << describe_object_comment(shape_name(obj->shape_type()), *obj, mat)
                        << "\n";
                out << "pl " << format_vec(pl->point) << ' ' << format_vec(pl->normal) << ' '
                        << format_rgba(mat.base_color, mat.alpha) << ' ' << mirror_token(mat)
                        << ' ' << move_token(*obj) << "\n";
                any = true;
        }
        if (!any)
                out << "# No walls\n";
}

std::string beam_move_token(const std::shared_ptr<BeamSource> &source)
{
        return (source && source->movable) ? "M" : "IM";
}

bool beam_has_laser(const std::shared_ptr<BeamSource> &source)
{
        return source && static_cast<bool>(source->beam);
}

void write_beams_section(std::ostream &out, const Scene &scene,
                                                 const std::vector<Material> &materials)
{
        out << "#-----------BEAMS-------------\n";
        bool any = false;
        for (const auto &obj : scene.objects)
        {
                if (!obj || !obj->is_beam())
                        continue;
                auto laser = std::static_pointer_cast<Laser>(obj);
                if (laser->start > 0.0)
                        continue;
                const Material &mat = materials[laser->material_id];
                auto source = laser->source.lock();
                out << "# Beam source (id: " << laser->object_id << ") color="
                        << describe_rgba(mat.base_color, mat.alpha)
                        << " intensity=" << format_scalar(laser->light_intensity)
                        << " movable=" << ((source && source->movable) ? "yes" : "no")
                        << "\n";
                out << "bm " << format_scalar(laser->light_intensity) << ' '
                        << format_vec(laser->path.orig) << ' ' << format_vec(laser->path.dir)
                        << ' ' << format_rgba(mat.base_color, mat.alpha) << ' '
                        << format_scalar(laser->radius) << ' '
                        << format_scalar(laser->total_length) << ' '
                        << beam_move_token(source) << ' '
                        << (beam_has_laser(source) ? "L" : "NL") << "\n";
                any = true;
        }

        for (const auto &obj : scene.objects)
        {
                if (!obj || obj->shape_type() != ShapeType::BeamTarget)
                        continue;
                auto target = std::static_pointer_cast<BeamTarget>(obj);
                const Material &mat = materials[target->material_id];
                out << "# Beam target (id: " << target->object_id << ") color="
                        << describe_rgba(mat.base_color, mat.alpha)
                        << " movable=" << (target->movable ? "yes" : "no") << "\n";
                out << "bt " << format_vec(target->center) << ' '
                        << format_rgba(mat.base_color, mat.alpha) << ' '
                        << format_scalar(target->radius) << ' '
                        << move_token(*target) << "\n";
                any = true;
        }

        if (!any)
                out << "# No beams\n";
}

} // namespace

bool MapSaver::save(const std::string &path, const Scene &scene, const Camera &cam,
                                        const std::vector<Material> &materials)
{
        std::ofstream out(path);
        if (!out)
                return false;

        const auto beam_related = collect_beam_related_ids(scene);

        write_camera_section(out, cam);
        out << '\n';
        write_lighting_section(out, scene);
        out << '\n';
        write_objects_section(out, scene, materials, beam_related);
        out << '\n';
        write_walls_section(out, scene, materials, beam_related);
        out << '\n';
        write_beams_section(out, scene, materials);

        return true;
}

