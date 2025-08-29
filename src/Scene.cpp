#include "rt/Scene.hpp"
#include "rt/Beam.hpp"
#include "rt/Parser.hpp"
#include <algorithm>

namespace rt
{
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

static inline Vec3 reflect(const Vec3 &v, const Vec3 &n)
{
  return v - n * (2.0 * Vec3::dot(v, n));
}

void Scene::update_beams()
{
  // Remove reflection segments
  objects.erase(std::remove_if(objects.begin(), objects.end(), [](const HittablePtr &o) {
                    if (o->is_beam())
                    {
                      Beam *b = static_cast<Beam *>(o.get());
                      return b->start > 0.0;
                    }
                    return false;
                  }),
                  objects.end());

  // Reset lengths of root beams
  for (auto &o : objects)
  {
    if (o->is_beam())
    {
      Beam *b = static_cast<Beam *>(o.get());
      b->length = b->total_length;
    }
  }

  int oid = next_object_id;
  const auto &materials = Parser::get_materials();

  size_t base_size = objects.size();
  for (size_t i = 0; i < base_size; ++i)
  {
    if (!objects[i]->is_beam())
      continue;
    Beam *bm = static_cast<Beam *>(objects[i].get());
    Ray forward(bm->path.orig, bm->path.dir);
    HitRecord tmp, hit_rec;
    bool hit_any = false;
    double closest = bm->length;
    for (auto &other : objects)
    {
      if (other.get() == bm)
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
      if (materials[hit_rec.material_id].mirror)
      {
        double new_start = bm->start + closest;
        double new_len = bm->total_length - new_start;
        if (new_len > 1e-4)
        {
          Vec3 refl_dir = reflect(forward.dir, hit_rec.normal);
          Vec3 refl_orig = forward.at(closest) + refl_dir * 1e-4;
          auto new_bm = std::make_shared<Beam>(refl_orig, refl_dir, bm->radius,
                                               new_len, oid++, bm->material_id,
                                               new_start, bm->total_length);
          objects.push_back(new_bm);
        }
      }
    }
  }

  next_object_id = oid;

  build_bvh();
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
