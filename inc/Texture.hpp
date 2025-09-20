#pragma once

#include "Vec3.hpp"
#include <string>
#include <vector>

class Texture
{
        public:
        bool load_from_file(const std::string &path);
        bool is_valid() const { return width > 0 && height > 0 && !pixels.empty(); }
        Vec3 sample(double u, double v) const;
        int get_width() const { return width; }
        int get_height() const { return height; }

        private:
        int width = 0;
        int height = 0;
        std::vector<Vec3> pixels;
        Vec3 get_pixel(int x, int y) const;
};

