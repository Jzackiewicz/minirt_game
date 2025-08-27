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
};

class Renderer
{
public:
  Renderer(const Scene &s, Camera &c);
  void render_ppm(const std::string &path, const std::vector<Material> &mats,
                  const RenderSettings &rset);
  void render_window(const std::vector<Material> &mats,
                     const RenderSettings &rset);

private:
  const Scene &scene;
  Camera &cam;
};

} // namespace rt
