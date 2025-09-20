#pragma once
#include "Vec3.hpp"
#include <string>
#include <vector>

struct Texture
{
        int width = 0;
        int height = 0;
        std::vector<Vec3> pixels;
        bool valid() const { return width > 0 && height > 0 && pixels.size() == static_cast<size_t>(width * height); }
        Vec3 sample(double u, double v) const;
};

bool load_xpm_texture(const std::string &path, Texture &out, std::string &error);
