#pragma once

#include "Camera.hpp"
#include "Scene.hpp"
#include "material.hpp"

#include <string>
#include <vector>

class MapSaver
{
        public:
        static bool save(const std::string &path, const Scene &scene,
                                          const Camera &camera,
                                          const std::vector<Material> &materials);
};
