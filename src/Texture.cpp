#include "Texture.hpp"
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace
{

bool parse_hex_color(const std::string &value, Vec3 &out)
{
        std::string hex = value;
        if (!hex.empty() && hex.front() == '#')
                hex.erase(hex.begin());
        if (hex.size() != 6 && hex.size() != 3)
                return false;
        auto hex_val = [](char c) -> int
        {
                if (c >= '0' && c <= '9')
                        return c - '0';
                if (c >= 'a' && c <= 'f')
                        return c - 'a' + 10;
                if (c >= 'A' && c <= 'F')
                        return c - 'A' + 10;
                return -1;
        };
        int r = 0, g = 0, b = 0;
        if (hex.size() == 6)
        {
                int vals[3] = {0, 0, 0};
                for (int i = 0; i < 3; ++i)
                {
                        int hi = hex_val(hex[i * 2]);
                        int lo = hex_val(hex[i * 2 + 1]);
                        if (hi < 0 || lo < 0)
                                return false;
                        vals[i] = (hi << 4) | lo;
                }
                r = vals[0];
                g = vals[1];
                b = vals[2];
        }
        else
        {
                int vals[3] = {0, 0, 0};
                for (int i = 0; i < 3; ++i)
                {
                        int v = hex_val(hex[i]);
                        if (v < 0)
                                return false;
                        vals[i] = (v << 4) | v;
                }
                r = vals[0];
                g = vals[1];
                b = vals[2];
        }
        out = Vec3(r / 255.0, g / 255.0, b / 255.0);
        return true;
}

struct XpmData
{
        int width = 0;
        int height = 0;
        int color_count = 0;
        int chars_per_pixel = 0;
        std::vector<std::string> entries;
};

bool read_xpm_entries(const std::string &path, XpmData &data)
{
        std::ifstream in(path);
        if (!in)
                return false;
        std::string line;
        while (std::getline(in, line))
        {
                size_t first = line.find('"');
                if (first == std::string::npos)
                        continue;
                size_t last = line.find('"', first + 1);
                if (last == std::string::npos)
                        continue;
                std::string entry = line.substr(first + 1, last - first - 1);
                if (!entry.empty())
                        data.entries.push_back(entry);
        }
        if (data.entries.empty())
                return false;
        std::istringstream header(data.entries.front());
        header >> data.width >> data.height >> data.color_count >> data.chars_per_pixel;
        if (!header || data.width <= 0 || data.height <= 0 || data.color_count <= 0 ||
            data.chars_per_pixel <= 0)
                return false;
        if (static_cast<int>(data.entries.size()) < 1 + data.color_count + data.height)
                return false;
        return true;
}

bool load_xpm(const std::string &path, Texture &out)
{
        XpmData data;
        if (!read_xpm_entries(path, data))
                return false;

        std::unordered_map<std::string, Vec3> palette;
        palette.reserve(static_cast<size_t>(data.color_count));
        for (int i = 0; i < data.color_count; ++i)
        {
                const std::string &entry = data.entries[1 + i];
                if (static_cast<int>(entry.size()) < data.chars_per_pixel)
                        return false;
                std::string key = entry.substr(0, data.chars_per_pixel);
                std::string desc = entry.substr(data.chars_per_pixel);
                std::istringstream iss(desc);
                std::string token;
                Vec3 color(0.0, 0.0, 0.0);
                bool has_color = false;
                while (iss >> token)
                {
                        if (token == "c" || token == "C")
                        {
                                if (!(iss >> token))
                                        return false;
                                if (token == "None" || token == "none")
                                {
                                        color = Vec3(0.0, 0.0, 0.0);
                                        has_color = true;
                                        break;
                                }
                                if (!parse_hex_color(token, color))
                                        return false;
                                has_color = true;
                                break;
                        }
                }
                if (!has_color)
                        return false;
                palette[key] = color;
        }

        out.width = data.width;
        out.height = data.height;
        out.pixels.assign(static_cast<size_t>(out.width * out.height), Vec3(0.0, 0.0, 0.0));
        for (int y = 0; y < out.height; ++y)
        {
                const std::string &row = data.entries[1 + data.color_count + y];
                if (static_cast<int>(row.size()) < data.chars_per_pixel * out.width)
                        return false;
                for (int x = 0; x < out.width; ++x)
                {
                        std::string key = row.substr(x * data.chars_per_pixel, data.chars_per_pixel);
                        auto it = palette.find(key);
                        Vec3 color = (it != palette.end()) ? it->second : Vec3(0.0, 0.0, 0.0);
                        out.pixels[static_cast<size_t>(y * out.width + x)] = color;
                }
        }
        return true;
}

} // namespace

Vec3 Texture::sample(double u, double v) const
{
        if (!valid())
                return Vec3(0.0, 0.0, 0.0);
        if (!std::isfinite(u) || !std::isfinite(v))
                return Vec3(0.0, 0.0, 0.0);
        double wrapped_u = u - std::floor(u);
        double wrapped_v = v - std::floor(v);
        if (wrapped_u < 0.0)
                wrapped_u += 1.0;
        if (wrapped_v < 0.0)
                wrapped_v += 1.0;
        double fx = wrapped_u * (width > 1 ? (width - 1) : 0);
        double fy = (1.0 - wrapped_v) * (height > 1 ? (height - 1) : 0);
        int x0 = static_cast<int>(std::floor(fx));
        int y0 = static_cast<int>(std::floor(fy));
        double dx = fx - x0;
        double dy = fy - y0;
        if (width <= 1)
        {
                x0 = 0;
                dx = 0.0;
        }
        if (height <= 1)
        {
                y0 = 0;
                dy = 0.0;
        }
        int x1 = (width > 1) ? (x0 + 1) % width : x0;
        int y1 = (height > 1) ? (y0 + 1) % height : y0;
        auto idx = [this](int ix, int iy) { return static_cast<size_t>(iy * width + ix); };
        Vec3 c00 = pixels[idx(x0, y0)];
        Vec3 c10 = pixels[idx(x1, y0)];
        Vec3 c01 = pixels[idx(x0, y1)];
        Vec3 c11 = pixels[idx(x1, y1)];
        Vec3 c0 = c00 * (1.0 - dx) + c10 * dx;
        Vec3 c1 = c01 * (1.0 - dx) + c11 * dx;
        return c0 * (1.0 - dy) + c1 * dy;
}

std::shared_ptr<Texture> load_texture(const std::string &path)
{
        auto tex = std::make_shared<Texture>();
        if (!load_xpm(path, *tex))
                return nullptr;
        return tex;
}

