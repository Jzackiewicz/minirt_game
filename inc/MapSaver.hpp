#pragma once

#include <string>
#include <vector>

class Camera;
class Scene;
class Material;

class MapSaver
{
        public:
        static bool save(const std::string &path, const Scene &scene,
                                         const Camera &cam,
                                         const std::vector<Material> &materials);
};

