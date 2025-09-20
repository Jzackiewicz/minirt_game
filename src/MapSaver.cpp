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
#include <cmath>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{

constexpr double kTransparentAlpha = 125.0 / 255.0;

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

std::string format_vec3_array(const Vec3 &v)
{
        return "[" + format_double(v.x) + ", " + format_double(v.y) + ", " + format_double(v.z) + "]";
}

int clamp_color_component(double value)
{
        double clamped = std::clamp(value, 0.0, 1.0);
        return static_cast<int>(std::round(clamped * 255.0));
}

std::string format_color_array(const Vec3 &color)
{
        int r = clamp_color_component(color.x);
        int g = clamp_color_component(color.y);
        int b = clamp_color_component(color.z);
        std::ostringstream oss;
        oss << "[" << r << ", " << g << ", " << b << "]";
        return oss.str();
}

std::string bool_str(bool value) { return value ? "true" : "false"; }

bool material_is_transparent(const Material &mat)
{
        return std::fabs(mat.alpha - kTransparentAlpha) < 1e-6;
}

std::string object_id_for(const std::string &prefix, int index)
{
        std::ostringstream oss;
        oss << prefix << index;
        return oss.str();
}

bool is_rotatable(const Hittable &obj)
{
        return obj.shape_type() != ShapeType::Plane;
}

struct PlaneRecord
{
        std::shared_ptr<Plane> plane;
        const Material *mat;
};

struct SphereRecord
{
        std::shared_ptr<Sphere> sphere;
        const Material *mat;
};

struct CubeRecord
{
        std::shared_ptr<Cube> cube;
        const Material *mat;
};

struct ConeRecord
{
        std::shared_ptr<Cone> cone;
        const Material *mat;
};

struct CylinderRecord
{
        std::shared_ptr<Cylinder> cylinder;
        const Material *mat;
};

} // namespace

bool MapSaver::save(const std::string &path, const Scene &scene, const Camera &camera,
                                         const std::vector<Material> &materials)
{
        std::ofstream out(path);
        if (!out)
                return false;

        out << "[camera]\n";
        out << "id = \"camera\"\n";
        out << "position = " << format_vec3_array(camera.origin) << "\n";
        out << "lookdir = " << format_vec3_array(camera.forward) << "\n";
        out << "fov = " << format_double(camera.fov_deg) << "\n\n";

        out << "[lighting.ambient]\n";
        out << "intensity = " << format_double(scene.ambient.intensity) << "\n";
        out << "color = " << format_color_array(scene.ambient.color) << "\n";

        int light_index = 1;
        for (const auto &light : scene.lights)
        {
                if (light.attached_id != -1)
                        continue;
                out << "\n[[lighting.light_sources]]\n";
                out << "id = \"light" << light_index++ << "\"\n";
                out << "intensity = " << format_double(light.intensity) << "\n";
                out << "position = " << format_vec3_array(light.position) << "\n";
                out << "color = " << format_color_array(light.color) << "\n";
        }
        out << "\n";

        std::vector<PlaneRecord> planes;
        std::vector<SphereRecord> spheres;
        std::vector<CubeRecord> cubes;
        std::vector<ConeRecord> cones;
        std::vector<CylinderRecord> cylinders;
        std::vector<std::shared_ptr<BeamSource>> beam_sources;
        std::vector<std::shared_ptr<BeamTarget>> beam_targets;

        for (const auto &obj : scene.objects)
        {
                if (auto laser = std::dynamic_pointer_cast<Laser>(obj))
                {
                        (void)laser;
                        continue;
                }
                if (auto source = std::dynamic_pointer_cast<BeamSource>(obj))
                {
                        beam_sources.push_back(source);
                        continue;
                }
                if (auto target = std::dynamic_pointer_cast<BeamTarget>(obj))
                {
                        beam_targets.push_back(target);
                        continue;
                }
                if (obj->material_id < 0 || obj->material_id >= static_cast<int>(materials.size()))
                        continue;
                const Material &mat = materials[obj->material_id];
                switch (obj->shape_type())
                {
                case ShapeType::Plane:
                        planes.push_back({std::static_pointer_cast<Plane>(obj), &mat});
                        break;
                case ShapeType::Sphere:
                        spheres.push_back({std::static_pointer_cast<Sphere>(obj), &mat});
                        break;
                case ShapeType::Cube:
                        cubes.push_back({std::static_pointer_cast<Cube>(obj), &mat});
                        break;
                case ShapeType::Cone:
                        cones.push_back({std::static_pointer_cast<Cone>(obj), &mat});
                        break;
                case ShapeType::Cylinder:
                        cylinders.push_back({std::static_pointer_cast<Cylinder>(obj), &mat});
                        break;
                default:
                        break;
                }
        }

        int plane_index = 1;
        for (const auto &rec : planes)
        {
                out << "[[objects.planes]]\n";
                out << "id = \"" << object_id_for("plane", plane_index++) << "\"\n";
                out << "color = " << format_color_array(rec.mat->base_color) << "\n";
                if (!rec.mat->texture_path.empty())
                        out << "texture = \"" << rec.mat->texture_path << "\"\n";
                out << "position = " << format_vec3_array(rec.plane->point) << "\n";
                out << "dir = " << format_vec3_array(rec.plane->normal.normalized()) << "\n";
                out << "reflective = " << bool_str(rec.mat->mirror) << "\n";
                out << "rotatable = " << bool_str(false) << "\n";
                out << "movable = " << bool_str(rec.plane->movable) << "\n";
                out << "scorable = " << bool_str(rec.plane->scorable) << "\n";
                out << "transparent = " << bool_str(material_is_transparent(*rec.mat)) << "\n\n";
        }

        int cube_index = 1;
        for (const auto &rec : cubes)
        {
                out << "[[objects.boxes]]\n";
                out << "id = \"" << object_id_for("box", cube_index++) << "\"\n";
                out << "color = " << format_color_array(rec.mat->base_color) << "\n";
                if (!rec.mat->texture_path.empty())
                        out << "texture = \"" << rec.mat->texture_path << "\"\n";
                out << "position = " << format_vec3_array(rec.cube->center) << "\n";
                out << "dir = " << format_vec3_array(rec.cube->axis[2]) << "\n";
                out << "width = " << format_double(rec.cube->half.y * 2.0) << "\n";
                out << "height = " << format_double(rec.cube->half.z * 2.0) << "\n";
                out << "length = " << format_double(rec.cube->half.x * 2.0) << "\n";
                out << "reflective = " << bool_str(rec.mat->mirror) << "\n";
                out << "rotatable = " << bool_str(is_rotatable(*rec.cube)) << "\n";
                out << "movable = " << bool_str(rec.cube->movable) << "\n";
                out << "scorable = " << bool_str(rec.cube->scorable) << "\n";
                out << "transparent = " << bool_str(material_is_transparent(*rec.mat)) << "\n\n";
        }

        int sphere_index = 1;
        for (const auto &rec : spheres)
        {
                out << "[[objects.spheres]]\n";
                out << "id = \"" << object_id_for("sphere", sphere_index++) << "\"\n";
                out << "color = " << format_color_array(rec.mat->base_color) << "\n";
                if (!rec.mat->texture_path.empty())
                        out << "texture = \"" << rec.mat->texture_path << "\"\n";
                out << "position = " << format_vec3_array(rec.sphere->center) << "\n";
                out << "dir = [0.0, 1.0, 0.0]\n";
                out << "radius = " << format_double(rec.sphere->radius) << "\n";
                out << "reflective = " << bool_str(rec.mat->mirror) << "\n";
                out << "rotatable = " << bool_str(is_rotatable(*rec.sphere)) << "\n";
                out << "movable = " << bool_str(rec.sphere->movable) << "\n";
                out << "scorable = " << bool_str(rec.sphere->scorable) << "\n";
                out << "transparent = " << bool_str(material_is_transparent(*rec.mat)) << "\n\n";
        }

        int cone_index = 1;
        for (const auto &rec : cones)
        {
                out << "[[objects.cones]]\n";
                out << "id = \"" << object_id_for("cone", cone_index++) << "\"\n";
                out << "color = " << format_color_array(rec.mat->base_color) << "\n";
                if (!rec.mat->texture_path.empty())
                        out << "texture = \"" << rec.mat->texture_path << "\"\n";
                out << "position = " << format_vec3_array(rec.cone->center) << "\n";
                out << "dir = " << format_vec3_array(rec.cone->axis) << "\n";
                out << "radius = " << format_double(rec.cone->radius) << "\n";
                out << "height = " << format_double(rec.cone->height) << "\n";
                out << "reflective = " << bool_str(rec.mat->mirror) << "\n";
                out << "rotatable = " << bool_str(is_rotatable(*rec.cone)) << "\n";
                out << "movable = " << bool_str(rec.cone->movable) << "\n";
                out << "scorable = " << bool_str(rec.cone->scorable) << "\n";
                out << "transparent = " << bool_str(material_is_transparent(*rec.mat)) << "\n\n";
        }

        int cylinder_index = 1;
        for (const auto &rec : cylinders)
        {
                out << "[[objects.cylinders]]\n";
                out << "id = \"" << object_id_for("cylinder", cylinder_index++) << "\"\n";
                out << "color = " << format_color_array(rec.mat->base_color) << "\n";
                if (!rec.mat->texture_path.empty())
                        out << "texture = \"" << rec.mat->texture_path << "\"\n";
                out << "position = " << format_vec3_array(rec.cylinder->center) << "\n";
                out << "dir = " << format_vec3_array(rec.cylinder->axis) << "\n";
                out << "radius = " << format_double(rec.cylinder->radius) << "\n";
                out << "height = " << format_double(rec.cylinder->height) << "\n";
                out << "reflective = " << bool_str(rec.mat->mirror) << "\n";
                out << "rotatable = " << bool_str(is_rotatable(*rec.cylinder)) << "\n";
                out << "movable = " << bool_str(rec.cylinder->movable) << "\n";
                out << "scorable = " << bool_str(rec.cylinder->scorable) << "\n";
                out << "transparent = " << bool_str(material_is_transparent(*rec.mat)) << "\n\n";
        }

        int beam_source_index = 1;
        for (const auto &source : beam_sources)
        {
                Vec3 direction = source->spot_direction();
                if (direction.length_squared() == 0.0)
                        direction = Vec3(0, 0, 1);
                Vec3 color = (source->beam && source->beam->color.length_squared() > 0.0)
                                     ? source->beam->color
                                     : (source->light ? source->light->color : Vec3(1, 1, 1));
                double intensity = source->beam ? source->beam->light_intensity
                                                : (source->light ? source->light->intensity : 0.0);
                double length = source->beam ? source->beam->total_length : 0.0;
                out << "[[beam.sources]]\n";
                out << "id = \"" << object_id_for("beam_source", beam_source_index++) << "\"\n";
                out << "intensity = " << format_double(intensity) << "\n";
                out << "position = " << format_vec3_array(source->center) << "\n";
                out << "dir = " << format_vec3_array(direction.normalized()) << "\n";
                out << "color = " << format_color_array(color) << "\n";
                out << "radius = " << format_double(source->radius) << "\n";
                out << "length = " << format_double(length) << "\n";
                out << "movable = " << bool_str(source->movable) << "\n";
                out << "rotatable = " << bool_str(true) << "\n";
                out << "scorable = " << bool_str(source->scorable) << "\n";
                out << "with_laser = " << bool_str(source->beam != nullptr) << "\n\n";
        }

        int beam_target_index = 1;
        for (const auto &target : beam_targets)
        {
                Vec3 color = Vec3(1, 1, 0);
                if (target->inner.material_id >= 0 &&
                    target->inner.material_id < static_cast<int>(materials.size()))
                        color = materials[target->inner.material_id].base_color;
                out << "[[beam.targets]]\n";
                out << "id = \"" << object_id_for("beam_target", beam_target_index++) << "\"\n";
                out << "position = " << format_vec3_array(target->center) << "\n";
                out << "color = " << format_color_array(color) << "\n";
                out << "radius = " << format_double(target->radius) << "\n";
                out << "movable = " << bool_str(target->movable) << "\n";
                out << "scorable = " << bool_str(target->scorable) << "\n\n";
        }

        return true;
}

