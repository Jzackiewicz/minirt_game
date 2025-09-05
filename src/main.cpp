
#include "rt/Camera.hpp"
#include "rt/Parser.hpp"   // single translation include for brevity
#include "rt/Renderer.hpp" // same trick
#include "rt/Scene.hpp"
#include <SDL.h>
#include <SDL_main.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>
#include <thread>

static bool point_in_rect(int x, int y, const SDL_Rect &r)
{
  return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

static void draw_button(SDL_Renderer *ren, TTF_Font *font, const SDL_Rect &rect,
                        const char *text, bool hover)
{
  SDL_Color fill = hover ? SDL_Color{0, 128, 128, 255}
                         : SDL_Color{0, 0, 0, 255};
  SDL_SetRenderDrawColor(ren, fill.r, fill.g, fill.b, fill.a);
  SDL_RenderFillRect(ren, &rect);
  SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
  for (int i = 0; i < 4; ++i)
  {
    SDL_Rect b = {rect.x - i, rect.y - i, rect.w + i * 2, rect.h + i * 2};
    SDL_RenderDrawRect(ren, &b);
  }
  SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text,
                                            SDL_Color{255, 255, 255, 255});
  if (surf)
  {
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    if (tex)
    {
      SDL_Rect dst = {rect.x + (rect.w - surf->w) / 2,
                      rect.y + (rect.h - surf->h) / 2, surf->w, surf->h};
      SDL_RenderCopy(ren, tex, nullptr, &dst);
      SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
  }
}

static bool show_main_menu(int width, int height)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
    return false;
  }
  if (TTF_Init() != 0)
  {
    std::cerr << "TTF_Init Error: " << TTF_GetError() << "\n";
    SDL_Quit();
    return false;
  }
  SDL_Window *win = SDL_CreateWindow("MiniRT", SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED, width, height, 0);
  if (!win)
  {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
    TTF_Quit();
    SDL_Quit();
    return false;
  }
  SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
  if (!ren)
  {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return false;
  }
  TTF_Font *font =
      TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 48);
  if (!font)
  {
    std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << "\n";
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return false;
  }
  SDL_Rect play = {width / 2 - 150, height / 2 - 100, 300, 80};
  SDL_Rect settings = {width / 2 - 150, height / 2 + 20, 300, 80};
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
        if (point_in_rect(mx, my, play))
        {
          start = true;
          running = false;
        }
        else if (point_in_rect(mx, my, settings))
        {
          // settings button does nothing yet
        }
      }
    }
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    bool hover_play = point_in_rect(mx, my, play);
    bool hover_set = point_in_rect(mx, my, settings);

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    draw_button(ren, font, play, "PLAY", hover_play);
    draw_button(ren, font, settings, "SETTINGS", hover_set);
    SDL_RenderPresent(ren);
  }

  TTF_CloseFont(font);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  TTF_Quit();
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

  unsigned int threads = std::thread::hardware_concurrency();
  if (threads == 0)
    threads = 8;

  float downscale = 1.0f;
  if (quality == 'M' || quality == 'm')
    downscale = 1.5f;
  else if (quality == 'L' || quality == 'l')
    downscale = 2.5f;
  if (!show_main_menu(width, height))
    return 0;

  rt::Scene scene;
  rt::Camera cam({0, 0, -10}, {0, 0, 0}, 60.0,
                 double(width) / double(height));
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
