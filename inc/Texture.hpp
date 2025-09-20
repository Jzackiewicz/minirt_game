#pragma once
#include "Vec3.hpp"
#include <memory>
#include <string>
#include <vector>

class Texture
{
        public:
        Texture() = default;

        Vec3 sample(double u, double v) const;
        int width() const { return width_; }
        int height() const { return height_; }
        bool empty() const { return width_ <= 0 || height_ <= 0 || pixels_.empty(); }

        private:
        int width_ = 0;
        int height_ = 0;
        std::vector<Vec3> pixels_;

        Vec3 texel(int x, int y) const;

        friend std::shared_ptr<Texture> load_texture(const std::string &path,
                                                     std::string &error);
};

std::shared_ptr<Texture> load_texture(const std::string &path, std::string &error);
