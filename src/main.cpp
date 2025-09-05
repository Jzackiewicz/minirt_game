
#include "rt/Camera.hpp"
#include "rt/Parser.hpp"   // single translation include for brevity
#include "rt/Renderer.hpp" // same trick
#include "rt/Scene.hpp"
#include <SDL.h>
#include <SDL_main.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// Simple 8x8 bitmap font for capital letters A-Z (public domain from
// https://github.com/dhepper/font8x8)
static const uint8_t FONT8X8[][8] = {
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // A
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // B
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // C
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // D
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // E
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // F
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // G
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // H
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // I
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // J
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // K
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // L
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // M
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // N
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // O
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // P
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // Q
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // R
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // S
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // T
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // U
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // V
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // W
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // X
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // Y
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}  // Z
};

static void draw_char(SDL_Renderer *ren, char c, int x, int y, int scale,
                      SDL_Color col)
{
  if (c < 'A' || c > 'Z')
    return;
  const uint8_t *bmp = FONT8X8[c - 'A'];
  SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
  for (int row = 0; row < 8; ++row)
  {
    for (int bit = 0; bit < 8; ++bit)
    {
      if (bmp[row] & (1 << bit))
      {
        SDL_Rect r{ x + bit * scale, y + row * scale, scale, scale };
        SDL_RenderFillRect(ren, &r);
      }
    }
  }
}

static void draw_text(SDL_Renderer *ren, const std::string &txt, int x, int y,
                      int scale, SDL_Color col)
{
  int offset = 0;
  for (char c : txt)
  {
    if (c == ' ')
    {
      offset += 8 * scale;
      continue;
    }
    draw_char(ren, std::toupper(c), x + offset, y, scale, col);
    offset += 8 * scale;
  }
}

static bool show_menu(int width, int height)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
    return false;
  }

  SDL_Window *win = SDL_CreateWindow("MiniRT", SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED, width, height, 0);
  if (!win)
  {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
    SDL_Quit();
    return false;
  }
  SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
  if (!ren)
  {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(win);
    SDL_Quit();
    return false;
  }

  SDL_Rect play_btn{ width / 2 - 150, height / 2 - 150, 300, 80 };
  SDL_Rect set_btn{ width / 2 - 150, height / 2 + 50, 300, 80 };

  bool running = true;
  bool start = false;
  SDL_Event e;
  while (running)
  {
    while (SDL_PollEvent(&e))
    {
      if (e.type == SDL_QUIT)
        running = false;
      else if (e.type == SDL_MOUSEBUTTONDOWN &&
               e.button.button == SDL_BUTTON_LEFT)
      {
        int mx = e.button.x;
        int my = e.button.y;
        if (mx >= play_btn.x && mx <= play_btn.x + play_btn.w &&
            my >= play_btn.y && my <= play_btn.y + play_btn.h)
        {
          start = true;
          running = false;
        }
        else if (mx >= set_btn.x && mx <= set_btn.x + set_btn.w &&
                 my >= set_btn.y && my <= set_btn.y + set_btn.h)
        {
          // Settings button: do nothing for now
        }
      }
      else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
      {
        running = false;
      }
    }

    int mx, my;
    SDL_GetMouseState(&mx, &my);
    bool play_hover = (mx >= play_btn.x && mx <= play_btn.x + play_btn.w &&
                       my >= play_btn.y && my <= play_btn.y + play_btn.h);
    bool set_hover = (mx >= set_btn.x && mx <= set_btn.x + set_btn.w &&
                      my >= set_btn.y && my <= set_btn.y + set_btn.h);

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);

    auto draw_button = [&](SDL_Rect rect, const std::string &text, bool hover) {
      SDL_Color fill = hover ? SDL_Color{0, 102, 102, 255}
                             : SDL_Color{0, 0, 0, 255};
      SDL_Color border{255, 255, 255, 255};
      SDL_Rect outer{rect.x - 5, rect.y - 5, rect.w + 10, rect.h + 10};
      SDL_SetRenderDrawColor(ren, border.r, border.g, border.b, border.a);
      SDL_RenderFillRect(ren, &outer);
      SDL_SetRenderDrawColor(ren, fill.r, fill.g, fill.b, fill.a);
      SDL_RenderFillRect(ren, &rect);
      int text_x = rect.x + (rect.w - (int)text.size() * 8 * 2) / 2;
      int text_y = rect.y + (rect.h - 8 * 2) / 2;
      draw_text(ren, text, text_x, text_y, 2, SDL_Color{255, 255, 255, 255});
    };

    draw_button(play_btn, "PLAY", play_hover);
    draw_button(set_btn, "SETTINGS", set_hover);

    SDL_RenderPresent(ren);
  }

  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();

  return start;
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

  if (!show_menu(width, height))
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
