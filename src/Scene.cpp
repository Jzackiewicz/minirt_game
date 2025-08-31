#include "rt/Scene.hpp"
#include "rt/Beam.hpp"
#include "rt/Plane.hpp"
#include "rt/Sphere.hpp"
#include "rt/Cube.hpp"
#include "rt/Cylinder.hpp"
#include "rt/Cone.hpp"
#include <algorithm>
#include <limits>

namespace
{
inline rt::Vec3 reflect(const rt::Vec3 &v, const rt::Vec3 &n)
{
  return v - n * (2.0 * rt::Vec3::dot(v, n));
}

inline bool sphere_sphere(const rt::Sphere &a, const rt::Sphere &b)
{
  rt::Vec3 diff = a.center - b.center;
  double rad = a.radius + b.radius;
  return diff.length_squared() <= rad * rad;
}

inline bool sphere_cube(const rt::Sphere &s, const rt::Cube &c)
{
  rt::Vec3 d = s.center - c.center;
  double local[3] = {rt::Vec3::dot(d, c.axis[0]), rt::Vec3::dot(d, c.axis[1]),
                     rt::Vec3::dot(d, c.axis[2])};
  double clamped[3];
  for (int i = 0; i < 3; ++i)
    clamped[i] = std::clamp(local[i], -c.half, c.half);
  rt::Vec3 closest = c.center + c.axis[0] * clamped[0] +
                     c.axis[1] * clamped[1] + c.axis[2] * clamped[2];
  rt::Vec3 diff = s.center - closest;
  return diff.length_squared() <= s.radius * s.radius;
}

inline bool object_plane(const rt::Hittable &obj, const rt::Plane &pl)
{
  rt::Vec3 pt = obj.support((-1.0) * pl.normal);
  double dist = rt::Vec3::dot(pt - pl.point, pl.normal);
  return dist <= 0.0;
}

inline rt::Vec3 support_minkowski(const rt::Hittable &a, const rt::Hittable &b,
                                  const rt::Vec3 &dir)
{
  return a.support(dir) - b.support((-1.0) * dir);
}

inline bool update_simplex(rt::Vec3 *simplex, int &n, rt::Vec3 &dir)
{
  using namespace rt;
  if (n == 2)
  {
    Vec3 a = simplex[1];
    Vec3 b = simplex[0];
    Vec3 ab = b - a;
    Vec3 ao = a * (-1.0);
    dir = Vec3::cross(Vec3::cross(ab, ao), ab);
  }
  else if (n == 3)
  {
    Vec3 a = simplex[2];
    Vec3 b = simplex[1];
    Vec3 c = simplex[0];
    Vec3 ab = b - a;
    Vec3 ac = c - a;
    Vec3 ao = a * (-1.0);
    Vec3 abc = Vec3::cross(ab, ac);
    Vec3 abp = Vec3::cross(abc, ab);
    if (Vec3::dot(abp, ao) > 0)
    {
      simplex[0] = a;
      simplex[1] = b;
      n = 2;
      dir = Vec3::cross(Vec3::cross(ab, ao), ab);
    }
    else
    {
      Vec3 acp = Vec3::cross(ac, abc);
      if (Vec3::dot(acp, ao) > 0)
      {
        simplex[1] = a;
        simplex[0] = c;
        n = 2;
        dir = Vec3::cross(Vec3::cross(ac, ao), ac);
      }
      else
      {
        if (Vec3::dot(abc, ao) > 0)
          dir = abc;
        else
        {
          std::swap(simplex[0], simplex[1]);
          dir = abc * -1.0;
        }
      }
    }
  }
  else if (n == 4)
  {
    Vec3 a = simplex[3];
    Vec3 b = simplex[2];
    Vec3 c = simplex[1];
    Vec3 d = simplex[0];
    Vec3 ao = a * (-1.0);
    Vec3 ab = b - a;
    Vec3 ac = c - a;
    Vec3 ad = d - a;
    Vec3 abc = Vec3::cross(ab, ac);
    Vec3 acd = Vec3::cross(ac, ad);
    Vec3 adb = Vec3::cross(ad, ab);
    if (Vec3::dot(abc, ao) > 0)
    {
      simplex[0] = c;
      simplex[1] = b;
      simplex[2] = a;
      n = 3;
      dir = abc;
    }
    else if (Vec3::dot(acd, ao) > 0)
    {
      simplex[0] = d;
      simplex[1] = c;
      simplex[2] = a;
      n = 3;
      dir = acd;
    }
    else if (Vec3::dot(adb, ao) > 0)
    {
      simplex[0] = b;
      simplex[1] = d;
      simplex[2] = a;
      n = 3;
      dir = adb;
    }
    else
    {
      return true;
    }
  }
  return false;
}

inline bool gjk(const rt::Hittable &a, const rt::Hittable &b)
{
  using namespace rt;
  Vec3 dir(1, 0, 0);
  Vec3 simplex[4];
  int n = 0;
  simplex[n++] = support_minkowski(a, b, dir);
  dir = simplex[0] * -1.0;
  for (int iter = 0; iter < 32; ++iter)
  {
    Vec3 pt = support_minkowski(a, b, dir);
    if (Vec3::dot(pt, dir) <= 0)
      return false;
    simplex[n++] = pt;
    if (update_simplex(simplex, n, dir))
      return true;
  }
  return false;
}

inline bool narrow_phase(const rt::Hittable &a, const rt::Hittable &b)
{
  if (a.is_plane())
    return object_plane(b, static_cast<const rt::Plane &>(a));
  if (b.is_plane())
    return object_plane(a, static_cast<const rt::Plane &>(b));
  if (a.kind() == rt::ShapeKind::Sphere && b.kind() == rt::ShapeKind::Sphere)
    return sphere_sphere(static_cast<const rt::Sphere &>(a),
                         static_cast<const rt::Sphere &>(b));
  if (a.kind() == rt::ShapeKind::Sphere && b.kind() == rt::ShapeKind::Cube)
    return sphere_cube(static_cast<const rt::Sphere &>(a),
                       static_cast<const rt::Cube &>(b));
  if (b.kind() == rt::ShapeKind::Sphere && a.kind() == rt::ShapeKind::Cube)
    return sphere_cube(static_cast<const rt::Sphere &>(b),
                       static_cast<const rt::Cube &>(a));
  return gjk(a, b);
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
    for (size_t i = 0; i < objects.size(); ++i)
    {
      if (static_cast<int>(i) == index)
        continue;
      auto other = objects[i];
      if (other->is_beam())
        continue;
      if (narrow_phase(*pl, *other))
        return true;
    }
    return false;
  }

  AABB box;
  if (!obj->bounding_box(box))
    return false;

  static thread_local std::vector<HittablePtr> candidates;
  candidates.clear();
  if (accel)
    std::static_pointer_cast<BVHNode>(accel)->query(box, candidates,
                                                   obj.get());
  else
  {
    for (size_t i = 0; i < objects.size(); ++i)
    {
      if (static_cast<int>(i) == index)
        continue;
      auto other = objects[i];
      if (other->is_beam() || other->is_plane())
        continue;
      candidates.push_back(other);
    }
  }

  for (auto &o : objects)
  {
    if (o->is_plane())
    {
      if (narrow_phase(*obj, *o))
        return true;
    }
  }

  for (auto &other : candidates)
  {
    if (narrow_phase(*obj, *other))
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
