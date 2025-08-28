#include "rt/Scene.hpp"

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

bool Scene::hit_solid(const Ray &r, double tmin, double tmax, HitRecord &rec) const
{
  bool hit_any = false;
  HitRecord tmp;
  double closest = tmax;
  for (auto &o : objects)
  {
    if (o->is_beam())
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
