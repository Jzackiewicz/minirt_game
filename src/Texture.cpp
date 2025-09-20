#include "Texture.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace
{
std::string to_lower_copy(std::string s)
{
        for (char &c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
}

Vec3 hex_to_vec3(const std::string &hex)
{
        if (hex.size() != 7 || hex[0] != '#')
                return Vec3(0.0, 0.0, 0.0);
        auto hex_value = [](char c) -> int {
                if (c >= '0' && c <= '9')
                        return c - '0';
                if (c >= 'a' && c <= 'f')
                        return 10 + (c - 'a');
                if (c >= 'A' && c <= 'F')
                        return 10 + (c - 'A');
                return 0;
        };
        int r = hex_value(hex[1]) * 16 + hex_value(hex[2]);
        int g = hex_value(hex[3]) * 16 + hex_value(hex[4]);
        int b = hex_value(hex[5]) * 16 + hex_value(hex[6]);
        return Vec3(r / 255.0, g / 255.0, b / 255.0);
}

bool parse_header(const std::string &line, int &width, int &height, int &colors,
                                  int &cpp)
{
        std::istringstream iss(line);
        if (!(iss >> width >> height >> colors >> cpp))
                return false;
        return width > 0 && height > 0 && colors > 0 && cpp > 0;
}

} // namespace

bool Texture::load_from_file(const std::string &path)
{
        std::string lower = to_lower_copy(path);
        if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".xpm")
                return load_xpm(path);
        return false;
}

Vec3 Texture::sample(double u, double v) const
{
        if (m_width <= 0 || m_height <= 0 || m_data.empty())
                return Vec3(0.0, 0.0, 0.0);
        return sample_impl(u, v);
}

Vec3 Texture::sample_impl(double u, double v) const
{
        if (m_width <= 0 || m_height <= 0)
                return Vec3(0.0, 0.0, 0.0);
        double fu = u - std::floor(u);
        double fv = v - std::floor(v);
        double x = fu * (m_width - 1);
        double y = (1.0 - fv) * (m_height - 1);
        int x0 = static_cast<int>(std::floor(x));
        int y0 = static_cast<int>(std::floor(y));
        int x1 = std::min(x0 + 1, m_width - 1);
        int y1 = std::min(y0 + 1, m_height - 1);
        double tx = x - x0;
        double ty = y - y0;
        auto at = [&](int ix, int iy) {
                size_t idx = static_cast<size_t>(iy) * static_cast<size_t>(m_width) +
                                      static_cast<size_t>(ix);
                if (idx >= m_data.size())
                        return Vec3(0.0, 0.0, 0.0);
                return m_data[idx];
        };
        Vec3 c00 = at(x0, y0);
        Vec3 c10 = at(x1, y0);
        Vec3 c01 = at(x0, y1);
        Vec3 c11 = at(x1, y1);
        Vec3 cx0 = c00 * (1.0 - tx) + c10 * tx;
        Vec3 cx1 = c01 * (1.0 - tx) + c11 * tx;
        return cx0 * (1.0 - ty) + cx1 * ty;
}

bool Texture::load_xpm(const std::string &path)
{
        std::ifstream file(path);
        if (!file)
                return false;
        std::vector<std::string> rows;
        std::string line;
        while (std::getline(file, line))
        {
                size_t start = line.find('"');
                size_t end = line.rfind('"');
                if (start == std::string::npos || end == std::string::npos || end <= start)
                        continue;
                rows.emplace_back(line.substr(start + 1, end - start - 1));
        }
        if (rows.empty())
                return false;
        int width = 0;
        int height = 0;
        int color_count = 0;
        int cpp = 0;
        if (!parse_header(rows[0], width, height, color_count, cpp))
                return false;
        if (static_cast<int>(rows.size()) < 1 + color_count + height)
                return false;
        std::unordered_map<std::string, Vec3> palette;
        palette.reserve(static_cast<size_t>(color_count));
        for (int i = 0; i < color_count; ++i)
        {
                const std::string &entry = rows[1 + i];
                if (static_cast<int>(entry.size()) < cpp)
                        return false;
                std::string key = entry.substr(0, cpp);
                size_t pos = entry.find('#');
                Vec3 color(0.0, 0.0, 0.0);
                if (pos != std::string::npos && pos + 7 <= entry.size())
                        color = hex_to_vec3(entry.substr(pos, 7));
                palette[key] = color;
        }
        std::vector<Vec3> data(static_cast<size_t>(width) * static_cast<size_t>(height));
        for (int y = 0; y < height; ++y)
        {
                const std::string &row = rows[1 + color_count + y];
                if (static_cast<int>(row.size()) < width * cpp)
                        return false;
                for (int x = 0; x < width; ++x)
                {
                        std::string key = row.substr(x * cpp, cpp);
                        auto it = palette.find(key);
                        Vec3 color(0.0, 0.0, 0.0);
                        if (it != palette.end())
                                color = it->second;
                        data[static_cast<size_t>(y) * static_cast<size_t>(width) +
                             static_cast<size_t>(x)] = color;
                }
        }
        m_width = width;
        m_height = height;
        m_data = std::move(data);
        return true;
}
