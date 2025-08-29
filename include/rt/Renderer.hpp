#pragma once
#include "Camera.hpp"
#include "Scene.hpp"
#include "material.hpp"
#include <string>
#include <vector>

namespace rt
{

struct RenderSettings
{
  int width = 800;
  int height = 600;
  int threads = 0; // 0 => auto
  int downscale = 1; // 1 => full res, 2 => half, 3 => third
};

class Renderer
{
public:
  Renderer(Scene &s, Camera &c);
  void render_ppm(const std::string &path, const std::vector<Material> &mats,
                  const RenderSettings &rset);
  void render_window(std::vector<Material> &mats,
                     const RenderSettings &rset);

private:
  Scene &scene;
  Camera &cam;
};

} // namespace rt
