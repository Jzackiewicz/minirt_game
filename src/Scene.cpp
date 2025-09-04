#include "rt/Scene.hpp"
#include "rt/Beam.hpp"
#include "rt/BeamSource.hpp"
#include "rt/Plane.hpp"
#include "rt/Collision.hpp"
#include "rt/Camera.hpp"
#include <algorithm>
#include <limits>
#include <cmath>
#include <unordered_map>

namespace rt
{
void Scene::update_beams(const std::vector<Material> &mats)
{
  std::vector<PointLight> static_lights;
  static_lights.reserve(lights.size());
  for (const auto &L : lights)
  {
    if (L.attached_id < 0)
      static_lights.push_back(L);
  }
  lights = std::move(static_lights);

  std::vector<std::shared_ptr<Beam>> roots;
  std::vector<HittablePtr> solids;
  std::vector<HittablePtr> visuals;
  std::unordered_map<int, int> id_map;

  for (auto &obj : objects)
  {
    if (obj->is_beam())
    {
      auto bm = std::static_pointer_cast<Beam>(obj);
      if (bm->start <= 0.0)
      {
        bm->start = 0.0;
        bm->length = bm->total_length;
        roots.push_back(bm);
      }
      continue;
    }
    if (!obj->collidable)
    {
      visuals.push_back(obj);
      continue;
    }
    solids.push_back(obj);
  }

  for (size_t i = 0; i < solids.size(); ++i)
  {
    id_map[solids[i]->object_id] = static_cast<int>(i);
    solids[i]->object_id = static_cast<int>(i);
  }

  objects = solids;
  int next_oid = static_cast<int>(objects.size());

  std::vector<std::shared_ptr<Beam>> to_process = roots;
  struct PendingLight
  {
    std::shared_ptr<Beam> beam;
    int hit_id;
  };
  std::vector<PendingLight> pending_lights;
  for (size_t i = 0; i < to_process.size(); ++i)
  {
    auto bm = to_process[i];
    if (i < roots.size())
      id_map[bm->object_id] = next_oid;
    bm->object_id = next_oid;
    bm->collidable = false;
    bm->casts_shadow = false;
    objects.push_back(bm);
    ++next_oid;

    Ray forward(bm->path.orig, bm->path.dir);
    HitRecord tmp, hit_rec;
    bool hit_any = false;
    double closest = bm->length;
    for (auto &other : objects)
    {
      if (other.get() == bm.get() || !other->collidable)
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
      if (mats[hit_rec.material_id].mirror)
      {
        double new_start = bm->start + closest;
        double new_len = bm->total_length - new_start;
        if (new_len > 1e-4)
        {
          Vec3 refl_dir = forward.dir -
                          hit_rec.normal *
                              (2.0 * Vec3::dot(forward.dir, hit_rec.normal));
          Vec3 refl_orig = forward.at(closest) + refl_dir * 1e-4;
          auto new_bm = std::make_shared<Beam>(
              refl_orig, refl_dir, bm->radius, new_len, bm->light_intensity, 0,
              bm->material_id, new_start, bm->total_length);
          new_bm->source = bm->source;
          to_process.push_back(new_bm);
          pending_lights.push_back({new_bm, hit_rec.object_id});
        }
      }
    }
    pending_lights.push_back({bm, -1});
  }

  for (const auto &pl : pending_lights)
  {
    auto bm = pl.beam;
    Vec3 light_col = mats[bm->material_id].base_color;
    double remain = bm->total_length - bm->start;
    double ratio = (bm->total_length > 0.0) ? remain / bm->total_length : 0.0;
    double cone_cos =
        std::cos(std::atan((bm->radius * 3.0) / std::max(1e-6, bm->length)));
    std::vector<int> ignore{bm->object_id};
    if (auto src = bm->source.lock())
      ignore.push_back(src->object_id);
    if (pl.hit_id >= 0)
      ignore.push_back(pl.hit_id);
    lights.emplace_back(bm->path.orig, light_col, bm->light_intensity * ratio,
                        ignore, bm->object_id, bm->path.dir, cone_cos,
                        bm->length);
  }

  for (auto &obj : objects)
  {
    auto it = id_map.find(obj->object_id);
    if (it != id_map.end())
      obj->object_id = it->second;
  }

  for (auto &vis : visuals)
  {
    vis->object_id = next_oid++;
    objects.push_back(vis);
  }
}
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

Vec3 Scene::move_with_collision(int index, const Vec3 &delta)
{
  if (index < 0 || index >= static_cast<int>(objects.size()))
    return Vec3(0, 0, 0);
  auto obj = objects[index];
  if (!obj || obj->is_beam() || !obj->collidable)
    return Vec3(0, 0, 0);

  auto move_lights = [&](const Vec3 &d) {
    for (auto &L : lights)
      if (L.attached_id == obj->object_id)
        L.position += d;
  };

  obj->translate(delta);
  move_lights(delta);
  if (!collides(index))
    return delta;
  obj->translate(delta * -1);
  move_lights(delta * -1);

  Vec3 moved(0, 0, 0);
  Vec3 axes[3] = {Vec3(delta.x, 0, 0), Vec3(0, delta.y, 0),
                  Vec3(0, 0, delta.z)};
  for (const Vec3 &ax : axes)
  {
    if (ax.length_squared() == 0)
      continue;
    obj->translate(ax);
    move_lights(ax);
    if (collides(index))
    {
      obj->translate(ax * -1);
      move_lights(ax * -1);
    }
    else
    {
      moved += ax;
    }
  }
  return moved;
}

Vec3 Scene::move_camera(Camera &cam, const Vec3 &delta,
                        const std::vector<Material> &mats) const
{
  auto blocked = [&](const Vec3 &start, const Vec3 &d) {
    double len = d.length();
    if (len <= 0.0)
      return false;
    Ray r(start, d / len);
    HitRecord tmp;
    for (const auto &obj : objects)
    {
      if (obj->is_beam() || !obj->collidable)
        continue;
      const Material &mat = mats[obj->material_id];
      if (mat.alpha < 1.0)
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

bool Scene::collides(int index) const
{
  if (index < 0 || index >= static_cast<int>(objects.size()))
    return false;
  auto obj = objects[index];
  if (obj->is_beam() || !obj->collidable)
    return false;
  if (obj->is_plane())
  {
    auto pl = std::static_pointer_cast<Plane>(obj);
    for (auto &other : objects)
    {
      if (other.get() == obj.get() || other->is_plane())
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
  }
  else
  {
    for (auto &o : objects)
      if (!o->is_plane() && o->collidable)
        candidates.push_back(o);
  }

  for (auto &cand : candidates)
  {
    if (cand.get() == obj.get() || !cand->collidable)
      continue;
    if (precise_collision(obj, cand))
      return true;
  }

  for (auto &o : objects)
  {
    if (!o->is_plane() || !o->collidable)
      continue;
    if (precise_collision(obj, o))
      return true;
  }

  return false;
}

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

} // namespace rt
