#pragma once

#include "Vec3.hpp"
#include <memory>
#include <string>
#include <vector>

class Texture
{
        public:
        int width = 0;
        int height = 0;
        std::vector<Vec3> pixels;

        bool valid() const
        {
                return width > 0 && height > 0 &&
                       pixels.size() == static_cast<size_t>(width * height);
        }

        Vec3 sample(double u, double v) const;
};

std::shared_ptr<Texture> load_texture(const std::string &path);

