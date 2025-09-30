#include "Scene.hpp"
#include "Camera.hpp"
#include "Collision.hpp"
#include "Laser.hpp"
#include "Plane.hpp"
#include "BeamTarget.hpp"
#include "Settings.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace
{
inline Vec3 reflect(const Vec3 &v, const Vec3 &n)
{
        return v - n * (2.0 * Vec3::dot(v, n));
}

Vec3 material_surface_color(const Material &mat, const HitRecord &rec)
{
        (void)rec;
        return mat.base_color;
}

} // namespace

// Remove lights attached to beam segments and collect root laser objects.
void Scene::prepare_beam_roots(std::vector<std::shared_ptr<Laser>> &roots,
                                                       std::unordered_map<int, int> &id_map)
{
        std::vector<PointLight> static_lights;
        static_lights.reserve(lights.size());
        for (const auto &L : lights)
        {
                bool keep = !L.reflected;
                if (keep && L.attached_id >= 0)
                {
                        auto it = std::find_if(objects.begin(), objects.end(),
                                                                   [&](const HittablePtr &o)
                                                                   { return o->object_id == L.attached_id; });
                        if (it != objects.end() && (*it)->is_beam())
                                keep = false;
                }
                if (keep)
                        static_lights.push_back(L);
        }
        lights = std::move(static_lights);

        std::vector<HittablePtr> non_beams;
        non_beams.reserve(objects.size());
        for (auto &obj : objects)
        {
                if (obj->is_beam())
                {
                        auto bm = std::static_pointer_cast<Laser>(obj);
                        if (bm->start <= 0.0)
                        {
                                bm->start = 0.0;
                                bm->length = bm->total_length;
                                roots.push_back(bm);
                        }
                        continue;
                }
                non_beams.push_back(obj);
        }

        for (size_t i = 0; i < non_beams.size(); ++i)
        {
                id_map[non_beams[i]->object_id] = static_cast<int>(i);
                non_beams[i]->object_id = static_cast<int>(i);
        }

        objects = std::move(non_beams);
}

// Trace laser beams, spawning reflections and associated point lights.
void Scene::process_beams(const std::vector<Material> &mats,
                                              std::vector<std::shared_ptr<Laser>> &roots,
                                              std::unordered_map<int, int> &id_map)
{
        for (auto &obj : objects)
                if (obj->shape_type() == ShapeType::BeamTarget)
                        std::static_pointer_cast<BeamTarget>(obj)->goal_active = false;
        int next_oid = static_cast<int>(objects.size());
        std::vector<std::shared_ptr<Laser>> to_process = roots;
        struct PendingLight
        {
                std::shared_ptr<Laser> beam;
                int hit_id;
                bool ignore_hit;
        };
        std::vector<PendingLight> pending_lights;
        for (size_t i = 0; i < to_process.size(); ++i)
        {
                auto bm = to_process[i];
                if (i < roots.size())
                        id_map[bm->object_id] = next_oid;
                bm->object_id = next_oid++;
                objects.push_back(bm);

                Ray forward(bm->path.orig, bm->path.dir);
                HitRecord tmp, hit_rec;
                bool hit_any = false;
                double closest = bm->length;
                for (auto &other : objects)
                {
                        if (other.get() == bm.get())
                                continue;
                        if (auto src = bm->source.lock())
                                if (other.get() == src.get())
                                        continue;
                        if (other->hit(forward, 1e-4, closest, tmp))
                        {
                                closest = tmp.t;
                                hit_rec = tmp;
                                hit_any = true;
                        }
                }
                if (hit_any)
                {
                        bm->length = closest;
                        std::shared_ptr<Hittable> hit_obj;
                        if (hit_rec.object_id >= 0 &&
                                hit_rec.object_id < static_cast<int>(objects.size()))
                        {
                                hit_obj = objects[hit_rec.object_id];
                                if (hit_obj->shape_type() == ShapeType::BeamTarget)
                                        std::static_pointer_cast<BeamTarget>(hit_obj)->start_goal();
                        }
                        bool block_transparent = false;
                        bool ignore_hit_for_light = true;
                        if (hit_obj)
                        {
                                block_transparent = hit_obj->blocks_when_transparent();
                                if (hit_obj->shape_type() == ShapeType::BeamTarget)
                                        ignore_hit_for_light = false;
                        }
                        const Material &hit_mat = mats[hit_rec.material_id];
                        if (hit_mat.mirror)
                        {
                                double new_start = bm->start + closest;
                                double new_len = bm->total_length - new_start;
                                if (new_len > 1e-4)
                                {
                                        Vec3 refl_dir = reflect(forward.dir, hit_rec.normal);
                                        Vec3 refl_orig = forward.at(closest) + refl_dir * 1e-4;
                                       auto new_bm = std::make_shared<Laser>(
                                               refl_orig, refl_dir, new_len,
                                               bm->light_intensity, 0, bm->material_id,
                                               new_start, bm->total_length);
                                        new_bm->color = bm->color;
                                        new_bm->scorable = bm->scorable;
                                        new_bm->source = bm->source;
                                        to_process.push_back(new_bm);
                                        pending_lights.push_back(
                                                {new_bm, hit_rec.object_id, ignore_hit_for_light});
                                }
                        }
                        else if (hit_mat.alpha < 1.0 && !block_transparent)
                        {
                                double new_start = bm->start + closest;
                                double new_len = bm->total_length - new_start;
                                if (new_len > 1e-4)
                                {
                                        Vec3 pass_orig = forward.at(closest) + forward.dir * 1e-4;
                                        Vec3 surface_col = material_surface_color(hit_mat, hit_rec);
                                        Vec3 new_color = bm->color * (1.0 - hit_mat.alpha) +
                                                         surface_col * hit_mat.alpha;
                                        double new_intens =
                                                bm->light_intensity * (1.0 - hit_mat.alpha);
                                       auto new_bm = std::make_shared<Laser>(
                                               pass_orig, forward.dir, new_len, new_intens, 0,
                                               bm->material_id, new_start, bm->total_length);
                                        new_bm->color = new_color;
                                        new_bm->scorable = bm->scorable;
                                        new_bm->source = bm->source;
                                        to_process.push_back(new_bm);
                                        pending_lights.push_back(
                                                {new_bm, hit_rec.object_id, ignore_hit_for_light});
                                }
                        }
                }
        }

        for (const auto &pl : pending_lights)
        {
                auto bm = pl.beam;
                Vec3 light_col = bm->color;
                const double cone_cos = std::sqrt(1.0 - 0.25 * 0.25);
                double remain = bm->total_length - bm->start;
                double ratio =
                        (bm->total_length > 0.0) ? remain / bm->total_length : 0.0;
                std::vector<int> ignore_ids{bm->object_id};
                if (pl.ignore_hit && pl.hit_id >= 0)
                        ignore_ids.push_back(pl.hit_id);
                lights.emplace_back(bm->path.orig, light_col,
                                                        bm->light_intensity * ratio,
                                                        ignore_ids, bm->object_id, bm->path.dir,
                                                        cone_cos, bm->length, false, true);
        }
}

// Remap light references after objects have been reindexed.
void Scene::remap_light_ids(const std::unordered_map<int, int> &id_map)
{
        for (auto &L : lights)
        {
                if (L.attached_id >= 0)
                {
                        auto it = id_map.find(L.attached_id);
                        if (it != id_map.end())
                                L.attached_id = it->second;
                        if (L.attached_id >= 0 &&
                                L.attached_id < static_cast<int>(objects.size()))
                        {
                                Vec3 dir = objects[L.attached_id]->spot_direction();
                                if (dir.length_squared() > 0)
                                        L.direction = dir.normalized();
                        }
                }
                for (int &ign : L.ignore_ids)
                {
                        auto it = id_map.find(ign);
                        if (it != id_map.end())
                                ign = it->second;
                }
        }
}

// Reflect directional and spotlight lights off mirror surfaces.
void Scene::reflect_lights(const std::vector<Material> &mats)
{
        struct LightSeg
        {
                PointLight L;
                double start;
                double total;
                int depth;
        };

        std::vector<LightSeg> to_process;
        to_process.reserve(lights.size());
        for (const auto &L : lights)
                to_process.push_back({L, 0.0, L.range, 0});

        lights.clear();
        const int max_bounce = 10;
        for (size_t i = 0; i < to_process.size(); ++i)
        {
                LightSeg seg = to_process[i];
                PointLight &L = seg.L;
                lights.push_back(L);
                if (L.range == 0.0 || L.direction.length_squared() == 0.0 ||
                        seg.depth >= max_bounce)
                        continue;
                Ray forward(L.position, L.direction.normalized());
                HitRecord tmp, hit_rec;
                bool hit_any = false;
                double closest = (L.range > 0.0) ? L.range : 1e9;
                for (auto &obj : objects)
                {
                        if (obj->is_beam())
                                continue;
                        if (L.attached_id == obj->object_id)
                                continue;
                        if (std::find(L.ignore_ids.begin(), L.ignore_ids.end(),
                                                  obj->object_id) != L.ignore_ids.end())
                                continue;
                        if (obj->hit(forward, 1e-4, closest, tmp))
                        {
                                closest = tmp.t;
                                hit_rec = tmp;
                                hit_any = true;
                        }
                }
                if (!hit_any || !mats[hit_rec.material_id].mirror)
                        continue;
                double new_start = seg.start + closest;
                double remain = (seg.total > 0.0) ? seg.total - new_start : -1.0;
                if (seg.total > 0.0 && remain <= 1e-4)
                        continue;
                Vec3 refl_dir = reflect(forward.dir, hit_rec.normal);
                Vec3 refl_orig = forward.at(closest) + refl_dir * 1e-4;
                double intensity = L.intensity;
                if (seg.total > 0.0)
                        intensity *= std::max(0.0, remain / seg.total);
                std::vector<int> ignore = L.ignore_ids;
                ignore.push_back(hit_rec.object_id);
                PointLight new_light(refl_orig, L.color, intensity, ignore, -1,
                                                         refl_dir, L.cutoff_cos, remain, true,
                                                         L.beam_spotlight, L.spot_radius);
                to_process.push_back({new_light, new_start, seg.total, seg.depth + 1});
        }
}

// Remove finished beam segments and spawn new beams for reflections.
void Scene::update_beams(const std::vector<Material> &mats)
{
        std::vector<std::shared_ptr<Laser>> roots;
        std::unordered_map<int, int> id_map;
        prepare_beam_roots(roots, id_map);
        process_beams(mats, roots, id_map);
        remap_light_ids(id_map);
        reflect_lights(mats);
}

void Scene::update_goal_targets(double dt, std::vector<Material> &mats)
{
        for (auto &obj : objects)
                if (obj->shape_type() == ShapeType::BeamTarget)
                        std::static_pointer_cast<BeamTarget>(obj)->update_goal(dt, mats);
}

// Construct a bounding volume hierarchy for faster ray queries.
void Scene::build_bvh()
{
	std::vector<HittablePtr> objs;
	objs.reserve(objects.size());
	for (auto &o : objects)
		if (!o->is_plane())
			objs.push_back(o);
	if (objs.empty())
	{
		accel.reset();
		return;
	}
	accel = std::make_shared<BVHNode>(objs, 0, objs.size());
}

// Move object by delta while preventing collisions.
Vec3 Scene::move_with_collision(int index, const Vec3 &delta)
{
        if (!is_movable(index))
        {
                return Vec3(0, 0, 0);
        }

        HittablePtr object;
        object = objects[index];

        if (g_developer_mode)
        {
                apply_translation(object, delta);
                return delta;
        }

        apply_translation(object, delta);
        if (!collides(index))
        {
                return delta;
        }
        apply_translation(object, delta * -1);

        Vec3 moved;
        moved = Vec3(0, 0, 0);

        Vec3 axis_deltas[3];
        axis_deltas[0] = Vec3(delta.x, 0, 0);
        axis_deltas[1] = Vec3(0, delta.y, 0);
        axis_deltas[2] = Vec3(0, 0, delta.z);
        for (const Vec3 &axis_delta : axis_deltas)
        {
                attempt_axis_move(index, axis_delta, moved);
        }
        return moved;
}

// Determine whether object is movable.
bool Scene::is_movable(int index) const
{
        if (index < 0 || index >= static_cast<int>(objects.size()))
        {
                return false;
        }
        HittablePtr object;
        object = objects[index];
        if (!object)
        {
                return false;
        }
        if (g_developer_mode)
        {
                return true;
        }
        if (object->is_beam())
        {
                return false;
        }
        return object->movable;
}

// Apply translation vector to given object.
void Scene::apply_translation(const HittablePtr &object, const Vec3 &delta)
{
	object->translate(delta);
	for (auto &light : lights)
	{
		if (light.attached_id == object->object_id)
		{
			light.position += delta;
		}
	}
}

// Try moving object along a single axis, updating moved vector on success.
void Scene::attempt_axis_move(int index, const Vec3 &axis_delta, Vec3 &moved)
{
	if (axis_delta.length_squared() == 0)
	{
		return;
	}
	HittablePtr object;
	object = objects[index];
	apply_translation(object, axis_delta);
	if (collides(index))
	{
		apply_translation(object, axis_delta * -1);
	}
	else
	{
		moved += axis_delta;
	}
}

// Move camera with collision avoidance.
Vec3 Scene::move_camera(Camera &cam, const Vec3 &delta,
                                                const std::vector<Material> &mats) const
{
        if (g_developer_mode)
        {
                cam.move(delta);
                return delta;
        }

        auto blocked = [&](const Vec3 &start, const Vec3 &d)
        {
                double len = d.length();
                if (len <= 0.0)
                        return false;
                Ray r(start, d / len);
                HitRecord tmp;
                for (const auto &obj : objects)
                {
                        if (obj->is_beam())
                                continue;
                        const Material &mat = mats[obj->material_id];
                        if (mat.alpha < 1.0 && !obj->blocks_when_transparent())
                                continue;
                        if (obj->hit(r, 1e-4, len, tmp))
                                return true;
                }
                return false;
        };

        Vec3 start = cam.origin;
        if (!blocked(start, delta))
        {
                cam.move(delta);
                return delta;
        }

        Vec3 moved(0, 0, 0);
        Vec3 axes[3] = {Vec3(delta.x, 0, 0), Vec3(0, delta.y, 0),
                                        Vec3(0, 0, delta.z)};
        for (const Vec3 &ax : axes)
        {
                if (ax.length_squared() == 0)
                        continue;
                if (!blocked(start, ax))
                {
                        cam.move(ax);
                        start += ax;
                        moved += ax;
                }
        }
        return moved;
}

// Check if object at index intersects any other object.
bool Scene::collides(int index) const
{
	if (index < 0 || index >= static_cast<int>(objects.size()))
		return false;
	auto obj = objects[index];
	if (obj->is_beam())
		return false;

	if (obj->is_plane())
	{
		auto pl = std::static_pointer_cast<Plane>(obj);
		for (auto &other : objects)
		{
			if (other.get() == obj.get() || other->is_beam() ||
				other->is_plane())
				continue;
			if (precise_collision(pl, other))
				return true;
		}
		return false;
	}

	AABB box;
	if (!obj->bounding_box(box))
		return false;

	std::vector<HittablePtr> candidates;
	candidates.reserve(16);
	if (accel && accel->is_bvh())
	{
		static_cast<BVHNode const *>(accel.get())->query(box, candidates);
		candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
										[](const HittablePtr &h)
										{ return h->is_beam(); }),
						 candidates.end());
	}
	else
	{
		for (auto &o : objects)
			if (!o->is_plane() && !o->is_beam())
				candidates.push_back(o);
	}

	for (auto &cand : candidates)
	{
		if (cand.get() == obj.get() || cand->is_beam())
			continue;
		if (precise_collision(obj, cand))
			return true;
	}

	for (auto &o : objects)
	{
		if (!o->is_plane())
			continue;
		if (precise_collision(obj, o))
			return true;
	}

	return false;
}

// Ray-scene intersection test.
bool Scene::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
	bool hit_any = false;
	HitRecord tmp;
	double closest = tmax;
	if (accel && accel->hit(r, tmin, tmax, tmp))
	{
		hit_any = true;
		closest = tmp.t;
		rec = tmp;
	}
	for (auto &o : objects)
	{
		if (!o->is_plane())
			continue;
		if (o->hit(r, tmin, closest, tmp))
		{
			hit_any = true;
			closest = tmp.t;
			rec = tmp;
		}
	}
	return hit_any;
}
