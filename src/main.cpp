
#include "rt/Camera.hpp"
#include "rt/Parser.hpp"   // single translation include for brevity
#include "rt/Renderer.hpp" // same trick
#include "rt/Scene.hpp"
#include <SDL.h>
#include <SDL_main.h>
#include <iostream>
#include <string>
#include <thread>

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

  rt::Scene scene;
  rt::Camera cam({0, 0, -10}, {0, 0, 0}, 60.0, double(width) / double(height));
  if (!rt::Parser::parse_rt_file(scene_path, scene, cam, width, height))
  {
    std::cerr << "Failed to parse scene: " << scene_path << "\n";
    return 2;
  }
  auto mats = rt::Parser::get_materials();
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
