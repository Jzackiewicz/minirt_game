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
using rt::Vec3;
using rt::Hittable;
using rt::Sphere;
using rt::Plane;

inline Vec3 reflect(const Vec3 &v, const Vec3 &n)
{
  return v - n * (2.0 * Vec3::dot(v, n));
}

struct Simplex
{
  Vec3 pts[4];
  int size = 0;
  void add(const Vec3 &p) { pts[size++] = p; }
  Vec3 &operator[](int i) { return pts[i]; }
};

Vec3 support(const Hittable &a, const Hittable &b, const Vec3 &dir)
{
  return a.support(dir) - b.support(dir * (-1.0));
}

bool handle_simplex(Simplex &s, Vec3 &dir)
{
  Vec3 a = s[s.size - 1];
  Vec3 ao = a * (-1.0);
  if (s.size == 2)
  {
    Vec3 b = s[0];
    Vec3 ab = b - a;
    dir = Vec3::cross(Vec3::cross(ab, ao), ab);
  }
  else if (s.size == 3)
  {
    Vec3 b = s[1];
    Vec3 c = s[0];
    Vec3 ab = b - a;
    Vec3 ac = c - a;
    Vec3 abc = Vec3::cross(ab, ac);
    Vec3 ab_perp = Vec3::cross(abc, ab);
    if (Vec3::dot(ab_perp, ao) > 0)
    {
      s[0] = b;
      s[1] = a;
      s.size = 2;
      dir = Vec3::cross(Vec3::cross(ab, ao), ab);
    }
    else
    {
      Vec3 ac_perp = Vec3::cross(ac, abc);
      if (Vec3::dot(ac_perp, ao) > 0)
      {
        s[1] = a;
        s.size = 2;
        dir = Vec3::cross(Vec3::cross(ac, ao), ac);
      }
      else
      {
        if (Vec3::dot(abc, ao) > 0)
          dir = abc;
        else
        {
          std::swap(s[0], s[1]);
          dir = abc * (-1.0);
        }
      }
    }
  }
  else if (s.size == 4)
  {
    Vec3 b = s[2];
    Vec3 c = s[1];
    Vec3 d = s[0];
    Vec3 ab = b - a;
    Vec3 ac = c - a;
    Vec3 ad = d - a;
    Vec3 abc = Vec3::cross(ab, ac);
    Vec3 acd = Vec3::cross(ac, ad);
    Vec3 adb = Vec3::cross(ad, ab);
    if (Vec3::dot(abc, ao) > 0)
    {
      s[0] = c;
      s[1] = b;
      s[2] = a;
      s.size = 3;
      dir = abc;
    }
    else if (Vec3::dot(acd, ao) > 0)
    {
      s[0] = d;
      s[1] = c;
      s[2] = a;
      s.size = 3;
      dir = acd;
    }
    else if (Vec3::dot(adb, ao) > 0)
    {
      s[0] = b;
      s[1] = d;
      s[2] = a;
      s.size = 3;
      dir = adb;
    }
    else
    {
      return true; // origin inside tetrahedron
    }
  }
  else
  {
    dir = Vec3(1, 0, 0);
  }
  return false;
}

bool gjk(const Hittable &a, const Hittable &b)
{
  Simplex s;
  Vec3 dir(1, 0, 0);
  s.add(support(a, b, dir));
  dir = s[0] * (-1.0);
  for (int i = 0; i < 20; ++i)
  {
    Vec3 p = support(a, b, dir);
    if (Vec3::dot(p, dir) < 0)
      return false;
    s.add(p);
    if (handle_simplex(s, dir))
      return true;
  }
  return false;
}

bool sphere_sphere(const Sphere &a, const Sphere &b)
{
  double r = a.radius + b.radius;
  return (a.center - b.center).length_squared() <= r * r;
}

bool plane_convex(const Plane &pl, const Hittable &obj)
{
  Vec3 p = obj.support(pl.normal * (-1.0));
  return Vec3::dot(p - pl.point, pl.normal) <= 0.0;
}

bool narrow_phase(const Hittable &a, const Hittable &b)
{
  if (a.is_sphere() && b.is_sphere())
  {
    const Sphere &sa = static_cast<const Sphere &>(a);
    const Sphere &sb = static_cast<const Sphere &>(b);
    return sphere_sphere(sa, sb);
  }
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
    for (auto &other : objects)
    {
      if (other.get() == obj.get() || other->is_beam() || other->is_plane())
        continue;
      if (plane_convex(*pl, *other))
        return true;
    }
    return false;
  }

  AABB box;
  if (!obj->bounding_box(box))
    return false;

  std::vector<HittablePtr> candidates;
  candidates.reserve(16);

  if (accel)
  {
    std::static_pointer_cast<BVHNode>(accel)->query(box, candidates, obj.get());
  }
  else
  {
    for (auto &other : objects)
    {
      if (other.get() == obj.get() || other->is_beam() || other->is_plane())
        continue;
      AABB ob;
      if (other->bounding_box(ob) && box.intersects(ob))
        candidates.push_back(other);
    }
  }

  for (auto &o : objects)
  {
    if (!o->is_plane())
      continue;
    auto pl = std::static_pointer_cast<Plane>(o);
    if (plane_convex(*pl, *obj))
      return true;
  }

  for (auto &cand : candidates)
  {
    if (narrow_phase(*obj, *cand))
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
