#include "rt/Scene.hpp"
#include "rt/Beam.hpp"
#include "rt/BeamSource.hpp"
#include "rt/Plane.hpp"
#include "rt/Collision.hpp"
#include "rt/Camera.hpp"
#include <algorithm>
#include <limits>
#include <unordered_set>
#include <cmath>

namespace
{
inline rt::Vec3 reflect(const rt::Vec3 &v, const rt::Vec3 &n)
{
  return v - n * (2.0 * rt::Vec3::dot(v, n));
}

} // namespace

namespace rt
{
void Scene::update_beams(const std::vector<Material> &mats)
{
  std::unordered_set<int> src_ids;
  for (const auto &bm : beams)
    if (auto src = bm->source.lock())
      src_ids.insert(src->object_id);

  std::vector<PointLight> static_lights;
  static_lights.reserve(lights.size());
  for (const auto &L : lights)
  {
    if (L.attached_id >= 0 && src_ids.count(L.attached_id))
      continue;
    static_lights.push_back(L);
  }
  lights = std::move(static_lights);

  std::vector<std::shared_ptr<Beam>> to_process = beams;
  for (size_t i = 0; i < to_process.size(); ++i)
  {
    auto bm = to_process[i];
    Ray forward(bm->path.orig, bm->path.dir);
    HitRecord tmp, hit_rec;
    bool hit_any = false;
    double max_len = bm->total_length - bm->start;
    double closest = max_len;
    for (auto &other : objects)
    {
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
    double segment_len = hit_any ? closest : max_len;
    bm->length = segment_len;

    Vec3 light_col = mats[bm->material_id].base_color;
    const double cone_cos = std::sqrt(1.0 - 0.25 * 0.25);
    double remain = bm->total_length - bm->start;
    double ratio = (bm->total_length > 0.0) ? remain / bm->total_length : 0.0;
    int src_id = -1;
    std::vector<int> ignore;
    if (auto src_ptr = bm->source.lock())
    {
      auto src = std::static_pointer_cast<BeamSource>(src_ptr);
      src_id = src->object_id;
      ignore.push_back(src->object_id);
      ignore.push_back(src->mid.object_id);
    }
    if (hit_any)
      ignore.push_back(hit_rec.object_id);
    lights.emplace_back(bm->path.orig, light_col, bm->light_intensity * ratio,
                        ignore, src_id, bm->path.dir, cone_cos, segment_len);

    if (hit_any && mats[hit_rec.material_id].mirror)
    {
      double new_start = bm->start + segment_len;
      double new_len = bm->total_length - new_start;
      if (new_len > 1e-4)
      {
        Vec3 refl_dir = reflect(forward.dir, hit_rec.normal);
        Vec3 refl_orig = forward.at(segment_len) + refl_dir * 1e-4;
        auto new_bm = std::make_shared<Beam>(
            refl_orig, refl_dir, new_len, bm->light_intensity, bm->material_id,
            new_start, bm->total_length);
        new_bm->source = bm->source;
        to_process.push_back(new_bm);
      }
    }
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
  if (!obj || obj->is_beam())
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
      if (obj->is_beam())
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
  if (obj->is_beam())
    return false;

  if (obj->is_plane())
  {
    auto pl = std::static_pointer_cast<Plane>(obj);
    for (auto &other : objects)
    {
      if (other.get() == obj.get() || other->is_beam() || other->is_plane())
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
                                    [](const HittablePtr &h) {
                                      return h->is_beam();
                                    }),
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
