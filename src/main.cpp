
#include "rt/Camera.hpp"
#include "rt/Parser.hpp"   // single translation include for brevity
#include "rt/Renderer.hpp" // same trick
#include "rt/Scene.hpp"
#include <SDL.h>
#include <SDL_main.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

static const uint8_t *get_glyph(char c)
{
  switch (c)
  {
  case 'A':
  {
    static const uint8_t data[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    return data;
  }
  case 'E':
  {
    static const uint8_t data[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
    return data;
  }
  case 'G':
  {
    static const uint8_t data[7] = {0x0F, 0x10, 0x10, 0x13, 0x11, 0x11, 0x0F};
    return data;
  }
  case 'I':
  {
    static const uint8_t data[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F};
    return data;
  }
  case 'L':
  {
    static const uint8_t data[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
    return data;
  }
  case 'N':
  {
    static const uint8_t data[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
    return data;
  }
  case 'P':
  {
    static const uint8_t data[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
    return data;
  }
  case 'S':
  {
    static const uint8_t data[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
    return data;
  }
  case 'T':
  {
    static const uint8_t data[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    return data;
  }
  case 'Y':
  {
    static const uint8_t data[7] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
    return data;
  }
  default:
    return nullptr;
  }
}

static void draw_char(SDL_Renderer *ren, char c, int x, int y, SDL_Color col,
                      int scale)
{
  const uint8_t *g = get_glyph(c);
  if (!g)
    return;
  SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
  for (int row = 0; row < 7; ++row)
  {
    for (int colb = 0; colb < 5; ++colb)
    {
      if (g[row] & (1 << (4 - colb)))
      {
        SDL_Rect r{ x + colb * scale, y + row * scale, scale, scale };
        SDL_RenderFillRect(ren, &r);
      }
    }
  }
}

static void draw_text(SDL_Renderer *ren, const std::string &txt, int x, int y,
                      SDL_Color col, int scale)
{
  for (char c : txt)
  {
    draw_char(ren, c, x, y, col, scale);
    x += (5 + 1) * scale;
  }
}

static bool show_main_menu(int width, int height)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    return false;
  SDL_Window *win = SDL_CreateWindow("MiniRT", SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED, width, height, 0);
  if (!win)
  {
    SDL_Quit();
    return false;
  }
  SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
  if (!ren)
  {
    SDL_DestroyWindow(win);
    SDL_Quit();
    return false;
  }

  SDL_Rect play_rect{width / 2 - 150, height / 2 - 150, 300, 100};
  SDL_Rect settings_rect{width / 2 - 150, height / 2 + 50, 300, 100};
  bool running = true;
  bool play_selected = false;
  while (running)
  {
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
      if (e.type == SDL_QUIT)
        running = false;
      else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
      {
        int mx = e.button.x, my = e.button.y;
        if (mx >= play_rect.x && mx < play_rect.x + play_rect.w &&
            my >= play_rect.y && my < play_rect.y + play_rect.h)
        {
          play_selected = true;
          running = false;
        }
      }
    }

    int mx, my;
    SDL_GetMouseState(&mx, &my);
    bool hover_play = mx >= play_rect.x && mx < play_rect.x + play_rect.w &&
                      my >= play_rect.y && my < play_rect.y + play_rect.h;
    bool hover_settings = mx >= settings_rect.x &&
                          mx < settings_rect.x + settings_rect.w &&
                          my >= settings_rect.y &&
                          my < settings_rect.y + settings_rect.h;

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);

    SDL_Color fill = hover_play ? SDL_Color{0, 128, 128, 255}
                                : SDL_Color{0, 0, 0, 255};
    SDL_SetRenderDrawColor(ren, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(ren, &play_rect);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderDrawRect(ren, &play_rect);
    int scale = 4;
    SDL_Color white{255, 255, 255, 255};
    auto get_text_w = [&](const std::string &s) {
      return (static_cast<int>(s.size()) * (5 + 1) - 1) * scale;
    };
    int tx = play_rect.x + (play_rect.w - get_text_w("PLAY")) / 2;
    int ty = play_rect.y + (play_rect.h - 7 * scale) / 2;
    draw_text(ren, "PLAY", tx, ty, white, scale);

    fill = hover_settings ? SDL_Color{0, 128, 128, 255}
                          : SDL_Color{0, 0, 0, 255};
    SDL_SetRenderDrawColor(ren, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(ren, &settings_rect);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderDrawRect(ren, &settings_rect);
    tx = settings_rect.x + (settings_rect.w - get_text_w("SETTINGS")) / 2;
    ty = settings_rect.y + (settings_rect.h - 7 * scale) / 2;
    draw_text(ren, "SETTINGS", tx, ty, white, scale);

    SDL_RenderPresent(ren);
    SDL_Delay(16);
  }

  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return play_selected;
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "Usage: minirt <scene.rt> [width height L|M|H]\n";
    return 1;
  }
  std::string scene_path = argv[1];

  char quality = 'H';
  if (argc > 2)
  {
    std::string last = argv[argc - 1];
    if (last.size() == 1 &&
        (last == "L" || last == "M" || last == "H" || last == "l" ||
         last == "m" || last == "h"))
    {
      quality = last[0];
      --argc;
    }
  }

  int width = (argc > 2) ? std::atoi(argv[2]) : 800;
  int height = (argc > 3) ? std::atoi(argv[3]) : 600;

  if (!show_main_menu(width, height))
    return 0;

  unsigned int threads = std::thread::hardware_concurrency();
  if (threads == 0)
    threads = 8;

  float downscale = 1.0f;
  if (quality == 'M' || quality == 'm')
    downscale = 1.5f;
  else if (quality == 'L' || quality == 'l')
    downscale = 2.5f;

  rt::Scene scene;
  rt::Camera cam({0, 0, -10}, {0, 0, 0}, 60.0, double(width) / double(height));
  if (!rt::Parser::parse_rt_file(scene_path, scene, cam, width, height))
  {
    std::cerr << "Failed to parse scene: " << scene_path << "\n";
    return 2;
  }
  auto mats = rt::Parser::get_materials();
  scene.update_beams(mats);
  scene.build_bvh();

  rt::RenderSettings rset;
  rset.width = width;
  rset.height = height;
  rset.threads = threads;
  rset.downscale = downscale;

  rt::Renderer renderer(scene, cam);
  renderer.render_window(mats, rset, scene_path);

  return 0;
}
