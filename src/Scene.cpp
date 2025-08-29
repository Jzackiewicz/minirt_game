#include "rt/Scene.hpp"
#include "rt/Beam.hpp"
#include <algorithm>

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
  if (objects.empty())
  {
    accel.reset();
    return;
  }
  std::vector<HittablePtr> objs = objects;
  accel = std::make_shared<BVHNode>(objs, 0, objs.size());
}

bool Scene::intersects_any(int index) const
{
  if (index < 0 || static_cast<size_t>(index) >= objects.size())
    return false;

  AABB box_i;
  if (!objects[index]->bounding_box(box_i))
    return false;

  for (size_t j = 0; j < objects.size(); ++j)
  {
    if (static_cast<int>(j) == index)
      continue;
    AABB box_j;
    if (!objects[j]->bounding_box(box_j))
      continue;
    if (box_i.intersects(box_j))
      return true;
  }
  return false;
}

bool Scene::hit(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  if (accel)
  {
    return accel->hit(r, tmin, tmax, rec);
  }
  bool hit_any = false;
  HitRecord tmp;
  double closest = tmax;
  for (auto &o : objects)
  {
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
