#include "rt/Renderer.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>

namespace rt
{

static bool in_shadow(const Scene &scene, const Vec3 &p, const Vec3 &light_pos)
{
  Vec3 dir = (light_pos - p).normalized();
  Ray shadow_ray(p + dir * 1e-4, dir);
  double dist_to_light = (light_pos - p).length();
  HitRecord tmp;
  for (const auto &obj : scene.objects)
  {
    if (obj->is_beam())
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
  if (depth > 10)
    return Vec3(0.0, 0.0, 0.0);
  HitRecord rec;
  if (!scene.hit(r, 1e-4, 1e9, rec))
  {
    return Vec3(0.0, 0.0, 0.0);
  }
  const Material &m = mats[rec.material_id];
  Vec3 eye = (r.dir * -1.0).normalized();
  Vec3 base = m.color;
  Vec3 sum(base.x * scene.ambient.color.x * scene.ambient.intensity,
           base.y * scene.ambient.color.y * scene.ambient.intensity,
           base.z * scene.ambient.color.z * scene.ambient.intensity);
  for (const auto &L : scene.lights)
  {
    if (in_shadow(scene, rec.p, L.position))
      continue;
    Vec3 ldir = (L.position - rec.p).normalized();
    double diff = std::max(0.0, Vec3::dot(rec.normal, ldir));
    Vec3 h = (ldir + eye).normalized();
    double spec =
        std::pow(std::max(0.0, Vec3::dot(rec.normal, h)), m.specular_exp) *
        m.specular_k;
    sum += Vec3(base.x * L.color.x * L.intensity * diff + L.color.x * spec,
                base.y * L.color.y * L.intensity * diff + L.color.y * spec,
                base.z * L.color.z * L.intensity * diff + L.color.z * spec);
  }
  if (m.mirror)
  {
    Vec3 refl_dir = r.dir - rec.normal * (2.0 * Vec3::dot(r.dir, rec.normal));
    Ray refl(rec.p + refl_dir * 1e-4, refl_dir);
    Vec3 refl_col = trace_ray(scene, mats, refl, rng, dist, depth + 1);
    double refl_ratio = REFLECTION / 100.0;
    sum = sum * (1.0 - refl_ratio) + refl_col * refl_ratio;
  }
  double alpha = m.alpha;
  if (m.random_alpha)
  {
    double tpos = std::clamp(rec.beam_ratio, 0.0, 1.0);
    double rand = (1.0 - tpos) * std::pow(dist(rng), tpos);
    alpha *= rand;
  }
  if (alpha < 1.0)
  {
    Ray next(rec.p + r.dir * 1e-4, r.dir);
    Vec3 behind = trace_ray(scene, mats, next, rng, dist, depth + 1);
    return sum * alpha + behind * (1.0 - alpha);
  }
  return sum;
}

static void move_with_collision(Camera &cam, const Scene &scene,
                                const Vec3 &delta)
{
  double len = delta.length();
  if (len < 1e-9)
    return;
  Ray r(cam.origin, delta / len);
  double tmin = 1e-4;
  double tmax = len;
  HitRecord rec;
  while (scene.hit(r, tmin, tmax, rec))
  {
    const auto &obj = scene.objects[rec.object_id];
    if (obj->is_beam() || !rec.front_face)
    {
      tmin = rec.t + 1e-4;
      continue;
    }
    double move_dist = rec.t - 1e-4;
    if (move_dist > 0.0)
      cam.move(r.dir * move_dist);
    return;
  }
  cam.move(delta);
}

Renderer::Renderer(const Scene &s, Camera &c) : scene(s), cam(c) {}

void Renderer::render_ppm(const std::string &path,
                          const std::vector<Material> &mats,
                          const RenderSettings &rset)
{
  const int W = rset.width;
  const int H = rset.height;
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

void Renderer::render_window(const std::vector<Material> &mats,
                             const RenderSettings &rset)
{
  const int W = rset.width;
  const int H = rset.height;
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
                                       SDL_TEXTUREACCESS_STREAMING, W, H);
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

  std::vector<Vec3> framebuffer(W * H);
  std::vector<unsigned char> pixels(W * H * 3);
  SDL_Event e;
  bool running = true;
  bool focused = false;
  Uint32 last = SDL_GetTicks();

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
      }
      else if (focused && e.type == SDL_MOUSEMOTION)
      {
        double sens = 0.002;
        cam.rotate(-e.motion.xrel * sens, -e.motion.yrel * sens);
      }
      else if (focused && e.type == SDL_KEYDOWN &&
               e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
        running = false;
    }

    const Uint8 *state = SDL_GetKeyboardState(nullptr);
    if (focused)
    {
      if (state[SDL_SCANCODE_ESCAPE])
        running = false;
      double speed = 15.0 * dt;
      double fmove = 0.0;
      if (state[SDL_SCANCODE_W])
        fmove += speed;
      if (state[SDL_SCANCODE_S])
        fmove -= speed;
      move_with_collision(cam, scene, cam.forward * fmove);
      double rmove = 0.0;
      if (state[SDL_SCANCODE_D])
        rmove += speed;
      if (state[SDL_SCANCODE_A])
        rmove -= speed;
      move_with_collision(cam, scene, cam.right * rmove);
    }

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

    for (int y = 0; y < H; ++y)
    {
      for (int x = 0; x < W; ++x)
      {
        Vec3 c = framebuffer[y * W + x];
        c.x = std::clamp(c.x, 0.0, 1.0);
        c.y = std::clamp(c.y, 0.0, 1.0);
        c.z = std::clamp(c.z, 0.0, 1.0);
        pixels[(y * W + x) * 3 + 0] =
            static_cast<unsigned char>(std::lround(c.x * 255.0));
        pixels[(y * W + x) * 3 + 1] =
            static_cast<unsigned char>(std::lround(c.y * 255.0));
        pixels[(y * W + x) * 3 + 2] =
            static_cast<unsigned char>(std::lround(c.z * 255.0));
      }
    }

    SDL_UpdateTexture(tex, nullptr, pixels.data(), W * 3);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);
  }

  SDL_DestroyTexture(tex);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}

} // namespace rt
