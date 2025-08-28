#include "rt/Scene.hpp"
#include "rt/Beam.hpp"

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

namespace
{
inline Vec3 reflect(const Vec3 &v, const Vec3 &n)
{
  return v - n * (2.0 * Vec3::dot(v, n));
}
} // namespace

void Scene::update_beams(const std::vector<Material> &mats)
{
  struct BeamDef
  {
    size_t index;
    Vec3 origin;
    Vec3 dir;
    double radius;
    double total_length;
    int material_id;
  };

  std::vector<HittablePtr> statics;
  std::vector<BeamDef> bases;

  for (size_t i = 0; i < objects.size(); ++i)
  {
    auto &obj = objects[i];
    if (obj->is_beam())
    {
      Beam *bm = static_cast<Beam *>(obj.get());
      if (bm->start == 0.0)
      {
        bases.push_back({i, bm->path.orig, bm->path.dir, bm->radius,
                         bm->total_length, bm->material_id});
      }
    }
    else
    {
      statics.push_back(obj);
    }
  }

  std::vector<HittablePtr> new_objs;
  new_objs.reserve(statics.size() + bases.size());
  size_t static_idx = 0;
  size_t base_idx = 0;

  for (size_t i = 0; i < objects.size(); ++i)
  {
    if (base_idx < bases.size() && bases[base_idx].index == i)
    {
      auto def = bases[base_idx++];
      double start = 0.0;
      double total = def.total_length;
      double remaining = total;
      Vec3 origin = def.origin;
      Vec3 dir = def.dir.normalized();

      while (remaining > 1e-4)
      {
        auto bm = std::make_shared<Beam>(origin, dir, def.radius, remaining,
                                         0, def.material_id, start, total);

        Ray forward(origin, dir);
        HitRecord tmp, hit_rec;
        bool hit_any = false;
        double closest = remaining;
        for (auto &o : statics)
        {
          if (o->hit(forward, 1e-4, closest, tmp))
          {
            closest = tmp.t;
            hit_rec = tmp;
            hit_any = true;
          }
        }
        if (hit_any)
          bm->length = closest;
        new_objs.push_back(bm);

        if (hit_any && mats[hit_rec.material_id].mirror)
        {
          start += closest;
          remaining = total - start;
          if (remaining <= 1e-4)
            break;
          Vec3 refl_dir = reflect(dir, hit_rec.normal);
          origin = forward.at(closest) + refl_dir * 1e-4;
          dir = refl_dir;
        }
        else
        {
          break;
        }
      }
    }
    else
    {
      new_objs.push_back(statics[static_idx++]);
    }
  }

  for (size_t i = 0; i < new_objs.size(); ++i)
    new_objs[i]->set_id(static_cast<int>(i));

  objects = std::move(new_objs);
}

} // namespace rt
