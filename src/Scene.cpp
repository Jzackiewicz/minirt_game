#include "rt/Scene.hpp"
#include "rt/Beam.hpp"
#include "rt/Plane.hpp"
#include "rt/Collision.hpp"
#include "rt/Camera.hpp"
#include "rt/Config.hpp"
#include <algorithm>
#include <limits>
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
  std::vector<std::shared_ptr<Beam>> roots;
  std::vector<HittablePtr> non_beams;
  non_beams.reserve(objects.size());

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
    non_beams.push_back(obj);
  }

  for (size_t i = 0; i < non_beams.size(); ++i)
    non_beams[i]->object_id = static_cast<int>(i);

  objects = std::move(non_beams);
  int next_oid = static_cast<int>(objects.size());

  std::vector<std::shared_ptr<Beam>> to_process = roots;
  for (size_t i = 0; i < to_process.size(); ++i)
  {
    auto bm = to_process[i];
    bm->object_id = next_oid;
    objects.push_back(bm);
    ++next_oid;

    Ray forward(bm->path.orig, bm->path.dir);
    HitRecord tmp, hit_rec;
    bool hit_any = false;
    double closest = bm->length;
    for (auto &other : objects)
    {
      if (other.get() == bm.get())
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
          Vec3 refl_dir = reflect(forward.dir, hit_rec.normal);
          Vec3 refl_orig = forward.at(closest) + refl_dir * 1e-4;
          auto new_bm = std::make_shared<Beam>(refl_orig, refl_dir, bm->radius,
                                               new_len, 0, bm->material_id,
                                               new_start, bm->total_length);
          to_process.push_back(new_bm);
        }
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

Vec3 Scene::move_with_collision(int index, const Vec3 &delta, const Camera &cam)
{
  if (index < 0 || index >= static_cast<int>(objects.size()))
    return Vec3(0, 0, 0);
  auto obj = objects[index];
  if (!obj || obj->is_beam())
    return Vec3(0, 0, 0);

  auto cam_overlap = [&]() {
    AABB box;
    if (!obj->bounding_box(box))
      return false;
    Vec3 center = (box.min + box.max) * 0.5;
    Vec3 half = (box.max - box.min) * 0.5;
    double radius = half.length() + CAMERA_OBJECT_MARGIN;
    return (cam.origin - center).length() < radius;
  };

  obj->translate(delta);
  if (!collides(index) && !cam_overlap())
    return delta;
  obj->translate(delta * -1);

  Vec3 moved(0, 0, 0);
  Vec3 axes[3] = {Vec3(delta.x, 0, 0), Vec3(0, delta.y, 0),
                  Vec3(0, 0, delta.z)};
  for (const Vec3 &ax : axes)
  {
    if (ax.length_squared() == 0)
      continue;
    obj->translate(ax);
    if (collides(index) || cam_overlap())
      obj->translate(ax * -1);
    else
      moved += ax;
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
    Vec3 dir = d / len;
    Ray r(start, dir);
    for (const auto &obj : objects)
    {
      if (obj->is_beam())
        continue;
      const Material &mat = mats[obj->material_id];
      if (mat.alpha < 1.0)
        continue;
      if (obj->is_plane())
      {
        HitRecord tmp;
        if (obj->hit(r, 1e-4, len, tmp))
          return true;
        continue;
      }
      AABB box;
      if (!obj->bounding_box(box))
        continue;
      Vec3 center = (box.min + box.max) * 0.5;
      Vec3 oc = start - center;
      double radius = ((box.max - box.min) * 0.5).length() +
                      CAMERA_OBJECT_MARGIN;
      double b = Vec3::dot(oc, dir);
      double c = oc.length_squared() - radius * radius;
      if (c <= 0.0)
        return true;
      double disc = b * b - c;
      if (disc >= 0.0)
      {
        double t = -b - std::sqrt(disc);
        if (t >= 0.0 && t <= len)
          return true;
      }
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
