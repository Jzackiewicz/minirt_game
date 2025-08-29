#include "rt/Renderer.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
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
  Vec3 sum(col.x * scene.ambient.color.x * scene.ambient.intensity,
           col.y * scene.ambient.color.y * scene.ambient.intensity,
           col.z * scene.ambient.color.z * scene.ambient.intensity);
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
    sum += Vec3(col.x * L.color.x * L.intensity * diff + L.color.x * spec,
                col.y * L.color.y * L.intensity * diff + L.color.y * spec,
                col.z * L.color.z * L.intensity * diff + L.color.z * spec);
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

static const char *hud_chars = "EDITSPCAORM";
static const uint8_t hud_font[][5] = {
    {0b11111, 0b10000, 0b11100, 0b10000, 0b11111}, // E
    {0b11110, 0b10001, 0b10001, 0b10001, 0b11110}, // D
    {0b11111, 0b00100, 0b00100, 0b00100, 0b11111}, // I
    {0b11111, 0b00100, 0b00100, 0b00100, 0b00100}, // T
    {0b01111, 0b10000, 0b01110, 0b00001, 0b11110}, // S
    {0b11110, 0b10001, 0b11110, 0b10000, 0b10000}, // P
    {0b01110, 0b10001, 0b10000, 0b10001, 0b01110}, // C
    {0b01110, 0b10001, 0b11111, 0b10001, 0b10001}, // A
    {0b01110, 0b10001, 0b10001, 0b10001, 0b01110}, // O
    {0b11110, 0b10001, 0b11110, 0b10100, 0b10010}, // R
    {0b10001, 0b11011, 0b10101, 0b10001, 0b10001}  // M
};

static void draw_char(SDL_Renderer *ren, char c, int x, int y, int scale)
{
  const char *p = std::strchr(hud_chars, c);
  if (!p)
    return;
  size_t idx = p - hud_chars;
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 7; ++j)
      if (hud_font[idx][i] & (1 << j))
      {
        SDL_Rect r{x + i * scale, y + j * scale, scale, scale};
        SDL_RenderFillRect(ren, &r);
      }
}

static void draw_text(SDL_Renderer *ren, const std::string &txt, int x, int y,
                      int scale)
{
  for (size_t i = 0; i < txt.size(); ++i)
  {
    if (txt[i] == ' ')
      continue;
    draw_char(ren, txt[i], x + static_cast<int>(i) * (6 * scale), y, scale);
  }
}

Renderer::Renderer(Scene &s, Camera &c) : scene(s), cam(c) {}

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

void Renderer::render_window(std::vector<Material> &mats,
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
  bool edit_mode = false;
  int hover_obj = -1;
  int hover_mat = -1;
  int selected_obj = -1;
  int selected_mat = -1;

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
      else if (e.type == SDL_KEYDOWN &&
               e.key.keysym.scancode == SDL_SCANCODE_R)
      {
        edit_mode = !edit_mode;
        if (!edit_mode)
        {
          if (hover_mat >= 0)
            mats[hover_mat].color = mats[hover_mat].base_color;
          hover_obj = hover_mat = -1;
          if (selected_mat >= 0)
          {
            mats[selected_mat].checkered = false;
            mats[selected_mat].color = mats[selected_mat].base_color;
          }
          selected_obj = selected_mat = -1;
        }
      }
      else if (e.type == SDL_MOUSEBUTTONDOWN &&
               e.button.button == SDL_BUTTON_LEFT)
      {
        focused = true;
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_ShowCursor(SDL_DISABLE);
        SDL_SetWindowGrab(win, SDL_TRUE);
        SDL_WarpMouseInWindow(win, W / 2, H / 2);
        if (edit_mode)
        {
          if (selected_obj == -1 && hover_obj >= 0)
          {
            selected_obj = hover_obj;
            selected_mat = hover_mat;
            mats[selected_mat].checkered = true;
            mats[selected_mat].color = mats[selected_mat].base_color;
          }
          else if (selected_obj != -1)
          {
            mats[selected_mat].checkered = false;
            mats[selected_mat].color = mats[selected_mat].base_color;
            selected_obj = selected_mat = -1;
          }
        }
      }
      else if (focused && e.type == SDL_MOUSEMOTION)
      {
        double sens = 0.002;
        if (edit_mode && selected_obj != -1)
        {
          scene.objects[selected_obj]->rotate(cam.up, -e.motion.xrel * sens);
          scene.objects[selected_obj]->rotate(cam.right, -e.motion.yrel * sens);
          scene.update_beams(mats);
          scene.build_bvh();
        }
        else
        {
          cam.rotate(-e.motion.xrel * sens, -e.motion.yrel * sens);
        }
      }
      else if (edit_mode && selected_obj != -1 && e.type == SDL_MOUSEWHEEL)
      {
        double step = e.wheel.y * 1.0;
        scene.objects[selected_obj]->translate(cam.up * step);
        scene.update_beams(mats);
        scene.build_bvh();
      }
      else if (focused && e.type == SDL_KEYDOWN &&
               e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
        running = false;
    }

    const Uint8 *state = SDL_GetKeyboardState(nullptr);
    if (edit_mode && selected_obj != -1)
    {
      double speed = 15.0 * dt;
      Vec3 move(0, 0, 0);
      if (state[SDL_SCANCODE_W])
        move += cam.forward * speed;
      if (state[SDL_SCANCODE_S])
        move += cam.forward * -speed;
      if (state[SDL_SCANCODE_A])
        move += cam.right * -speed;
      if (state[SDL_SCANCODE_D])
        move += cam.right * speed;
      if (move.length_squared() > 0)
      {
        scene.objects[selected_obj]->translate(move);
        scene.update_beams(mats);
        scene.build_bvh();
      }
    }
    else if (focused)
    {
      if (state[SDL_SCANCODE_ESCAPE])
        running = false;
      double speed = 15.0 * dt;
      if (state[SDL_SCANCODE_W])
        cam.move(cam.forward * speed);
      if (state[SDL_SCANCODE_S])
        cam.move(cam.forward * -speed);
      if (state[SDL_SCANCODE_A])
        cam.move(cam.right * -speed);
      if (state[SDL_SCANCODE_D])
        cam.move(cam.right * speed);
    }

    if (edit_mode && selected_obj == -1)
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
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    draw_text(ren, edit_mode ? "EDIT" : "SPECTATOR", 5, 5, 2);
    SDL_RenderPresent(ren);
  }

  SDL_DestroyTexture(tex);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}

} // namespace rt
