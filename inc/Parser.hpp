#pragma once
#include "Camera.hpp"
#include "Scene.hpp"
#include "material.hpp"
#include <string>
#include <vector>

namespace rt
{

class Parser
{
public:
  static bool parse_rt_file(const std::string &path, Scene &outScene,
                            Camera &outCamera, int width, int height);

  static const std::vector<Material> &get_materials();
  static bool save_rt_file(const std::string &path, const Scene &scene,
                           const Camera &cam,
                           const std::vector<Material> &mats);

private:
  // TO JEST KLUCZOWE: deklaracja storage'u
  static std::vector<Material> materials;
};

} // namespace rt
