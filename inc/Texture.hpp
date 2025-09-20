#pragma once

#include "Vec3.hpp"
#include <memory>
#include <string>
#include <vector>

class Texture
{
        public:
        Texture() = default;
        bool load_from_file(const std::string &path);
        Vec3 sample(double u, double v) const;
        int width() const { return m_width; }
        int height() const { return m_height; }

        private:
        int m_width = 0;
        int m_height = 0;
        std::vector<Vec3> m_data;

        bool load_xpm(const std::string &path);
        Vec3 sample_impl(double u, double v) const;
};

using TexturePtr = std::shared_ptr<Texture>;
