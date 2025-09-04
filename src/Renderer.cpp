#include "rt/Renderer.hpp"
#include "rt/Config.hpp"
#include "rt/AABB.hpp"
#include "rt/Parser.hpp"
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
#include <random>
#include <thread>
#include <filesystem>
#include <cctype>

namespace rt
{

static bool in_shadow(const Scene &scene, const Vec3 &p, const PointLight &L)
{
  Vec3 to_light = L.position - p;
  double dist_to_light = to_light.length();
  if (L.range > 0.0 && dist_to_light > L.range)
    return false;
  Vec3 dir = to_light.normalized();
  Ray shadow_ray(p + dir * 1e-4, dir);
  HitRecord tmp;
  for (const auto &obj : scene.objects)
  {
    if (obj->is_beam() || !obj->casts_shadow)
      continue;
    if (std::find(L.ignore_ids.begin(), L.ignore_ids.end(), obj->object_id) !=
        L.ignore_ids.end())
      continue;
    if (obj->hit(shadow_ray, 1e-4, dist_to_light - 1e-4, tmp))
    {
      return true;
    }
  }
  return false;
}

static Vec3 trace_ray(const Scene &scene, const std::vector<Material> &mats,
                      const Ray &r, std::mt19937 &rng,
                      std::uniform_real_distribution<double> &dist,
                      int depth = 0)
{
  (void)rng;
  (void)dist;
  if (depth > 10)
    return Vec3(0.0, 0.0, 0.0);
  HitRecord rec;
  if (!scene.hit(r, 1e-4, 1e9, rec))
  {
    return Vec3(0.0, 0.0, 0.0);
  }
  const Material &m = mats[rec.material_id];
  Vec3 eye = (r.dir * -1.0).normalized();
  Vec3 base = m.base_color;
  Vec3 col = m.color;
  if (m.checkered)
  {
    Vec3 inv = Vec3(1.0, 1.0, 1.0) - base;
    int chk = (static_cast<int>(std::floor(rec.p.x * 5)) +
               static_cast<int>(std::floor(rec.p.y * 5)) +
               static_cast<int>(std::floor(rec.p.z * 5))) & 1;
    col = chk ? base : inv;
  }

  Vec3 sum;
  if (m.unlit)
  {
    sum = col;
  }
  else
  {
    sum = Vec3(col.x * scene.ambient.color.x * scene.ambient.intensity,
               col.y * scene.ambient.color.y * scene.ambient.intensity,
               col.z * scene.ambient.color.z * scene.ambient.intensity);
    for (const auto &L : scene.lights)
    {
      if (std::find(L.ignore_ids.begin(), L.ignore_ids.end(), rec.object_id) !=
          L.ignore_ids.end())
        continue;
      Vec3 to_light = L.position - rec.p;
      double dist = to_light.length();
      if (L.range > 0.0 && dist > L.range)
        continue;
      Vec3 ldir = to_light / dist;
      if (L.cutoff_cos > -1.0)
      {
        Vec3 spot_dir = (rec.p - L.position).normalized();
        if (Vec3::dot(L.direction, spot_dir) < L.cutoff_cos)
          continue;
      }
      if (in_shadow(scene, rec.p, L))
        continue;
      double atten = 1.0;
      if (L.range > 0.0)
        atten = std::max(0.0, 1.0 - dist / L.range);
      double diff = std::max(0.0, Vec3::dot(rec.normal, ldir));
      Vec3 h = (ldir + eye).normalized();
      double spec =
          std::pow(std::max(0.0, Vec3::dot(rec.normal, h)), m.specular_exp) *
          m.specular_k;
      sum += Vec3(col.x * L.color.x * L.intensity * diff * atten +
                      L.color.x * spec * atten,
                  col.y * L.color.y * L.intensity * diff * atten +
                      L.color.y * spec * atten,
                  col.z * L.color.z * L.intensity * diff * atten +
                      L.color.z * spec * atten);
    }
    if (m.mirror)
    {
      Vec3 refl_dir =
          r.dir - rec.normal * (2.0 * Vec3::dot(r.dir, rec.normal));
      Ray refl(rec.p + refl_dir * 1e-4, refl_dir);
      Vec3 refl_col = trace_ray(scene, mats, refl, rng, dist, depth + 1);
      double refl_ratio = REFLECTION / 100.0;
      sum = sum * (1.0 - refl_ratio) + refl_col * refl_ratio;
    }
  }

  double alpha = m.alpha;
  double tpos = std::clamp(rec.beam_ratio, 0.0, 1.0);
  alpha *= (1.0 - tpos);
  if (alpha < 1.0)
  {
    Ray next(rec.p + r.dir * 1e-4, r.dir);
    Vec3 behind = trace_ray(scene, mats, next, rng, dist, depth + 1);
    return sum * alpha + behind * (1.0 - alpha);
  }
  return sum;
}


static std::string next_save_path(const std::string &orig)
{
  namespace fs = std::filesystem;
  fs::path p(orig);
  fs::path dir = p.parent_path();
  std::string stem = p.stem().string();
  int base_id = 0;
  auto pos = stem.find_last_of('_');
  if (pos != std::string::npos)
  {
    bool num = true;
    for (size_t i = pos + 1; i < stem.size(); ++i)
      if (!std::isdigit(static_cast<unsigned char>(stem[i])))
      {
        num = false;
        break;
      }
    if (num)
    {
      base_id = std::atoi(stem.c_str() + pos + 1);
      stem = stem.substr(0, pos);
    }
  }
  int idx = base_id + 1;
  fs::path ext = p.extension();
  for (;; ++idx)
  {
    fs::path candidate = dir / (stem + "_" + std::to_string(idx) + ext.string());
    std::error_code ec;
    if (!fs::exists(candidate, ec))
      return candidate.string();
  }
}

Renderer::Renderer(Scene &s, Camera &c) : scene(s), cam(c) {}

void Renderer::render_ppm(const std::string &path,
                          const std::vector<Material> &mats,
                          const RenderSettings &rset)
{
  const float scale = std::max(1.0f, rset.downscale);
  const int W = std::max(1, static_cast<int>(rset.width / scale));
  const int H = std::max(1, static_cast<int>(rset.height / scale));
  const int T = (rset.threads > 0)
                    ? rset.threads
                    : (std::thread::hardware_concurrency()
                           ? (int)std::thread::hardware_concurrency()
                           : 8);

  std::vector<Vec3> framebuffer(W * H);
  std::atomic<int> next_row{0};

  auto worker = [&]()
  {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (;;)
    {
      int y = next_row.fetch_add(1);
      if (y >= H)
        break;
      for (int x = 0; x < W; ++x)
      {
        double u = (x + 0.5) / W;
        double v = (y + 0.5) / H;
        Ray r = cam.ray_through(u, v);
        Vec3 col = trace_ray(scene, mats, r, rng, dist);
        framebuffer[y * W + x] = col;
      }
    }
  };

  std::vector<std::thread> pool;
  pool.reserve(T);
  for (int i = 0; i < T; ++i)
    pool.emplace_back(worker);
  for (auto &th : pool)
    th.join();

  std::ofstream out(path, std::ios::binary);
  out << "P6\n" << W << " " << H << "\n255\n";
  for (int y = 0; y < H; ++y)
  {
    for (int x = 0; x < W; ++x)
    {
      Vec3 c = framebuffer[y * W + x];
      c.x = std::clamp(c.x, 0.0, 1.0);
      c.y = std::clamp(c.y, 0.0, 1.0);
      c.z = std::clamp(c.z, 0.0, 1.0);
      unsigned char r = static_cast<unsigned char>(std::lround(c.x * 255.0));
      unsigned char g = static_cast<unsigned char>(std::lround(c.y * 255.0));
      unsigned char b = static_cast<unsigned char>(std::lround(c.z * 255.0));
      out.put(r).put(g).put(b);
    }
  }
}

void Renderer::render_window(std::vector<Material> &mats,
                             const RenderSettings &rset,
                             const std::string &scene_path)
{
  const int W = rset.width;
  const int H = rset.height;
  const float scale = std::max(1.0f, rset.downscale);
  const int RW = std::max(1, static_cast<int>(W / scale));
  const int RH = std::max(1, static_cast<int>(H / scale));
  const int T = (rset.threads > 0)
                    ? rset.threads
                    : (std::thread::hardware_concurrency()
                           ? (int)std::thread::hardware_concurrency()
                           : 8);

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
    return;
  }
  SDL_Window *win =
      SDL_CreateWindow("MiniRT", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, W, H, 0);
  if (!win)
  {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
    SDL_Quit();
    return;
  }
  SDL_SetWindowResizable(win, SDL_FALSE);
  SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
  if (!ren)
  {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(win);
    SDL_Quit();
    return;
  }
  SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24,
                                       SDL_TEXTUREACCESS_STREAMING, RW, RH);
  if (!tex)
  {
    std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << "\n";
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return;
  }

  SDL_SetRelativeMouseMode(SDL_FALSE);

  SDL_ShowCursor(SDL_ENABLE);
  SDL_SetWindowGrab(win, SDL_FALSE);

  std::vector<Vec3> framebuffer(RW * RH);
  std::vector<unsigned char> pixels(RW * RH * 3);
  SDL_Event e;
  bool running = true;
  bool focused = false;
  Uint32 last = SDL_GetTicks();
  bool edit_mode = false;
  int hover_obj = -1;
  int hover_mat = -1;
  int selected_obj = -1;
  int selected_mat = -1;
  double edit_dist = 0.0;
  Vec3 edit_pos;
  bool rotating = false;

  while (running)
  {
    Uint32 now = SDL_GetTicks();
    double dt = (now - last) / 1000.0;
    last = now;


    while (SDL_PollEvent(&e))
    {
      if (e.type == SDL_QUIT)
        running = false;
      else if (e.type == SDL_WINDOWEVENT &&
               e.window.event == SDL_WINDOWEVENT_LEAVE)
      {
        focused = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_SetWindowGrab(win, SDL_FALSE);
      }
      else if (e.type == SDL_MOUSEBUTTONDOWN &&
               e.button.button == SDL_BUTTON_LEFT)
      {
        focused = true;
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_ShowCursor(SDL_DISABLE);
        SDL_SetWindowGrab(win, SDL_TRUE);
        SDL_WarpMouseInWindow(win, W / 2, H / 2);
        if (!edit_mode && hover_obj >= 0)
        {
          selected_obj = hover_obj;
          selected_mat = hover_mat;
          hover_obj = hover_mat = -1;
          mats[selected_mat].checkered = true;
          mats[selected_mat].color = mats[selected_mat].base_color;
          AABB box;
          if (scene.objects[selected_obj]->bounding_box(box))
          {
            Vec3 center = (box.min + box.max) * 0.5;
            edit_dist = (center - cam.origin).length();
            Vec3 desired = cam.origin + cam.forward * edit_dist;
            Vec3 delta = desired - center;
            if (delta.length_squared() > 0)
            {
              Vec3 applied = scene.move_with_collision(selected_obj, delta);
              center += applied;
              if (applied.length_squared() > 0)
              {
                scene.update_beams(mats);
                scene.build_bvh();
                edit_dist = (center - cam.origin).length();
              }
            }
            edit_pos = center;
          }
          edit_mode = true;
        }
        else if (edit_mode)
        {
          mats[selected_mat].checkered = false;
          mats[selected_mat].color = mats[selected_mat].base_color;
          selected_obj = selected_mat = -1;
          edit_mode = false;
          rotating = false;
        }
      }
      else if (e.type == SDL_MOUSEBUTTONDOWN &&
               e.button.button == SDL_BUTTON_RIGHT)
      {
        if (edit_mode)
          rotating = true;
      }
      else if (e.type == SDL_MOUSEBUTTONUP &&
               e.button.button == SDL_BUTTON_RIGHT)
      {
        rotating = false;
      }
      else if (focused && e.type == SDL_MOUSEMOTION)
      {
        if (edit_mode && rotating)
        {
          double sens = MOUSE_SENSITIVITY;
          bool changed = false;
          double yaw = -e.motion.xrel * sens;
          if (yaw != 0.0)
          {
            scene.objects[selected_obj]->rotate(cam.up, yaw);
            if (scene.collides(selected_obj))
              scene.objects[selected_obj]->rotate(cam.up, -yaw);
            else
              changed = true;
          }
          double pitch = -e.motion.yrel * sens;
          if (pitch != 0.0)
          {
            scene.objects[selected_obj]->rotate(cam.right, pitch);
            if (scene.collides(selected_obj))
              scene.objects[selected_obj]->rotate(cam.right, -pitch);
            else
              changed = true;
          }
          if (changed)
          {
            scene.update_beams(mats);
            scene.build_bvh();
          }
        }
        else
        {
          double sens = MOUSE_SENSITIVITY;
          cam.rotate(-e.motion.xrel * sens, -e.motion.yrel * sens);
        }
      }
      else if (e.type == SDL_MOUSEWHEEL)
      {
        double step = e.wheel.y * SCROLL_STEP;
        if (edit_mode)
        {
          edit_dist += step;
          if (edit_dist < 0.1)
            edit_dist = 0.1;
        }
        else if (focused)
        {
          scene.move_camera(cam, cam.up * step, mats);
        }
      }
      else if (focused && e.type == SDL_KEYDOWN &&
               e.key.keysym.scancode == SDL_SCANCODE_C)
      {
        scene.update_beams(mats);
        scene.build_bvh();
        std::string save = next_save_path(scene_path);
        if (Parser::save_rt_file(save, scene, cam, mats))
          std::cout << "Saved scene to: " << save << "\n";
        else
          std::cerr << "Failed to save scene to: " << save << "\n";
      }
      else if (focused && e.type == SDL_KEYDOWN &&
               e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
        running = false;
    }

    const Uint8 *state = SDL_GetKeyboardState(nullptr);
    Vec3 forward_xz = cam.forward;
    forward_xz.y = 0.0;
    if (forward_xz.length_squared() > 0.0)
      forward_xz = forward_xz.normalized();
    Vec3 right_xz = cam.right; // already horizontal

    if (edit_mode)
    {
      double cam_speed = CAMERA_MOVE_SPEED * dt;
      if (state[SDL_SCANCODE_W])
        scene.move_camera(cam, forward_xz * cam_speed, mats);
      if (state[SDL_SCANCODE_S])
        scene.move_camera(cam, forward_xz * -cam_speed, mats);
      if (state[SDL_SCANCODE_A])
        scene.move_camera(cam, right_xz * -cam_speed, mats);
      if (state[SDL_SCANCODE_D])
        scene.move_camera(cam, right_xz * cam_speed, mats);
      if (state[SDL_SCANCODE_SPACE])
        scene.move_camera(cam, Vec3(0, 1, 0) * cam_speed, mats);
      if (state[SDL_SCANCODE_LCTRL])
        scene.move_camera(cam, Vec3(0, -1, 0) * cam_speed, mats);

      double rot_speed = OBJECT_ROTATE_SPEED * dt;
      bool changed = false;
      if (state[SDL_SCANCODE_Q])
      {
        scene.objects[selected_obj]->rotate(cam.forward, -rot_speed);
        if (scene.collides(selected_obj))
          scene.objects[selected_obj]->rotate(cam.forward, rot_speed);
        else
          changed = true;
      }
      if (state[SDL_SCANCODE_E])
      {
        scene.objects[selected_obj]->rotate(cam.forward, rot_speed);
        if (scene.collides(selected_obj))
          scene.objects[selected_obj]->rotate(cam.forward, -rot_speed);
        else
          changed = true;
      }
      if (changed)
      {
        scene.update_beams(mats);
        scene.build_bvh();
      }
    }
    else if (focused)
    {
      if (state[SDL_SCANCODE_ESCAPE])
        running = false;
      double speed = CAMERA_MOVE_SPEED * dt;
      if (state[SDL_SCANCODE_W])
        scene.move_camera(cam, forward_xz * speed, mats);
      if (state[SDL_SCANCODE_S])
        scene.move_camera(cam, forward_xz * -speed, mats);
      if (state[SDL_SCANCODE_A])
        scene.move_camera(cam, right_xz * -speed, mats);
      if (state[SDL_SCANCODE_D])
        scene.move_camera(cam, right_xz * speed, mats);
      if (state[SDL_SCANCODE_SPACE])
        scene.move_camera(cam, Vec3(0, 1, 0) * speed, mats);
      if (state[SDL_SCANCODE_LCTRL])
        scene.move_camera(cam, Vec3(0, -1, 0) * speed, mats);
    }

    if (edit_mode)
    {
      Vec3 desired = cam.origin + cam.forward * edit_dist;
      Vec3 delta = desired - edit_pos;
      if (delta.length_squared() > 0)
      {
        Vec3 applied = scene.move_with_collision(selected_obj, delta);
        edit_pos += applied;
        cam.origin = edit_pos - cam.forward * edit_dist;
        if (applied.length_squared() > 0)
        {
          scene.update_beams(mats);
          scene.build_bvh();
        }
      }
      else
      {
        cam.origin = edit_pos - cam.forward * edit_dist;
      }
    }

    if (!edit_mode)
    {
      Ray center_ray = cam.ray_through(0.5, 0.5);
      HitRecord hrec;
      if (scene.hit(center_ray, 1e-4, 1e9, hrec) &&
          scene.objects[hrec.object_id]->movable)
      {
        if (hover_mat != hrec.material_id)
        {
          if (hover_mat >= 0)
            mats[hover_mat].color = mats[hover_mat].base_color;
          hover_obj = hrec.object_id;
          hover_mat = hrec.material_id;
        }
        bool blink = ((SDL_GetTicks() / 250) % 2) == 0;
        mats[hover_mat].color =
            blink ? (Vec3(1.0, 1.0, 1.0) - mats[hover_mat].base_color)
                  : mats[hover_mat].base_color;
      }
      else
      {
        if (hover_mat >= 0)
          mats[hover_mat].color = mats[hover_mat].base_color;
        hover_obj = hover_mat = -1;
      }
    }
    else
    {
      if (hover_mat >= 0 && hover_mat != selected_mat)
        mats[hover_mat].color = mats[hover_mat].base_color;
      hover_obj = hover_mat = -1;
    }

    std::atomic<int> next_row{0};
    auto worker = [&]()
    {
      std::mt19937 rng(std::random_device{}());
      std::uniform_real_distribution<double> dist(0.0, 1.0);
      for (;;)
      {
        int y = next_row.fetch_add(1);
        if (y >= RH)
          break;
        for (int x = 0; x < RW; ++x)
        {
          double u = (x + 0.5) / RW;
          double v = (y + 0.5) / RH;
          Ray r = cam.ray_through(u, v);
          Vec3 col = trace_ray(scene, mats, r, rng, dist);
          framebuffer[y * RW + x] = col;
        }
      }
    };

    std::vector<std::thread> pool;
    pool.reserve(T);
    for (int i = 0; i < T; ++i)
      pool.emplace_back(worker);
    for (auto &th : pool)
      th.join();

    for (int y = 0; y < RH; ++y)
    {
      for (int x = 0; x < RW; ++x)
      {
        Vec3 c = framebuffer[y * RW + x];
        c.x = std::clamp(c.x, 0.0, 1.0);
        c.y = std::clamp(c.y, 0.0, 1.0);
        c.z = std::clamp(c.z, 0.0, 1.0);
        pixels[(y * RW + x) * 3 + 0] =
            static_cast<unsigned char>(std::lround(c.x * 255.0));
        pixels[(y * RW + x) * 3 + 1] =
            static_cast<unsigned char>(std::lround(c.y * 255.0));
        pixels[(y * RW + x) * 3 + 2] =
            static_cast<unsigned char>(std::lround(c.z * 255.0));
      }
    }

    SDL_UpdateTexture(tex, nullptr, pixels.data(), RW * 3);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    if (edit_mode)
    {
      auto project = [&](const Vec3 &p, int &sx, int &sy) -> bool
      {
        Vec3 rel = p - cam.origin;
        double z = Vec3::dot(rel, cam.forward);
        if (z <= 0.0)
          return false;
        double x = Vec3::dot(rel, cam.right);
        double y = Vec3::dot(rel, cam.up);
        double fov_rad = cam.fov_deg * M_PI / 180.0;
        double half_h = std::tan(fov_rad * 0.5);
        double half_w = cam.aspect * half_h;
        double u = (x / z / half_w + 1.0) * 0.5;
        double v = (1.0 - y / z / half_h) * 0.5;
        sx = static_cast<int>(u * W);
        sy = static_cast<int>(v * H);
        return true;
      };
      const int edges[12][2] = {{0, 1}, {0, 2}, {0, 4}, {1, 3}, {1, 5},
                                {2, 3}, {2, 6}, {3, 7}, {4, 5}, {4, 6},
                                {5, 7}, {6, 7}};
      for (size_t i = 0; i < scene.objects.size(); ++i)
      {
        auto &obj = scene.objects[i];
        if (obj->is_beam() || obj->is_plane())
          continue;
        AABB box;
        if (!obj->bounding_box(box))
          continue;
        Vec3 corners[8] = {Vec3(box.min.x, box.min.y, box.min.z),
                           Vec3(box.max.x, box.min.y, box.min.z),
                           Vec3(box.min.x, box.max.y, box.min.z),
                           Vec3(box.max.x, box.max.y, box.min.z),
                           Vec3(box.min.x, box.min.y, box.max.z),
                           Vec3(box.max.x, box.min.y, box.max.z),
                           Vec3(box.min.x, box.max.y, box.max.z),
                           Vec3(box.max.x, box.max.y, box.max.z)};
        SDL_Point pts[8];
        bool ok = true;
        for (int j = 0; j < 8; ++j)
        {
          int sx, sy;
          if (!project(corners[j], sx, sy))
          {
            ok = false;
            break;
          }
          pts[j].x = sx;
          pts[j].y = sy;
        }
        if (!ok)
          continue;
        if (static_cast<int>(i) == selected_obj)
          SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
        else
          SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
        for (const auto &e : edges)
        {
          SDL_RenderDrawLine(ren, pts[e[0]].x, pts[e[0]].y, pts[e[1]].x,
                             pts[e[1]].y);
        }
      }
    }
    SDL_RenderPresent(ren);
  }

  SDL_DestroyTexture(tex);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}

} // namespace rt
