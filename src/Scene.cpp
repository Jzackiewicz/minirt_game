#include "rt/Scene.hpp"
#include "rt/Beam.hpp"
#include "rt/Cone.hpp"
#include "rt/Cylinder.hpp"
#include "rt/Plane.hpp"
#include "rt/Sphere.hpp"
#include <algorithm>
#include <functional>

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

void Scene::update_beams(const std::vector<Material> &materials)
{
  // collect non-beam objects and beam sources
  std::vector<HittablePtr> static_objs;
  std::vector<std::shared_ptr<Beam>> sources;
  for (auto &obj : objects)
  {
    if (obj->is_beam())
    {
      Beam *bm = static_cast<Beam *>(obj.get());
      if (bm->start < 1e-6)
      {
        bm->length = bm->total_length;
        sources.push_back(std::static_pointer_cast<Beam>(obj));
      }
    }
    else
    {
      static_objs.push_back(obj);
    }
  }

  objects = static_objs;

  auto reflect = [](const Vec3 &v, const Vec3 &n) {
    return v - n * (2.0 * Vec3::dot(v, n));
  };

  static int next_oid = 100000;

  std::function<void(const Vec3 &, const Vec3 &, double, double, double, int)>
      shoot = [&](const Vec3 &orig, const Vec3 &dir, double radius,
                  double start, double total, int mat_id) {
        Ray forward(orig, dir.normalized());
        HitRecord tmp, hit_rec;
        bool hit_any = false;
        double remaining = total - start;
        double closest = remaining;
        for (auto &other : static_objs)
        {
          if (other->hit(forward, 1e-4, closest, tmp))
          {
            closest = tmp.t;
            hit_rec = tmp;
            hit_any = true;
          }
        }

        auto bm = std::make_shared<Beam>(orig, dir, radius,
                                         hit_any ? closest : remaining,
                                         next_oid++, mat_id, start, total);
        objects.push_back(bm);

        if (hit_any && materials[hit_rec.material_id].mirror)
        {
          double new_start = start + closest;
          double new_len = total - new_start;
          if (new_len > 1e-4)
          {
            Vec3 refl_dir = reflect(forward.dir, hit_rec.normal);
            Vec3 refl_orig = forward.at(closest) + refl_dir * 1e-4;
            shoot(refl_orig, refl_dir, radius, new_start, total, mat_id);
          }
        }
      };

  for (auto &src : sources)
  {
    shoot(src->path.orig, src->path.dir, src->radius, 0.0, src->total_length,
          src->material_id);
  }
}

} // namespace rt
