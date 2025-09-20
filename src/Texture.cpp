#include "Texture.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace
{

int hex_value(char c)
{
        if (c >= '0' && c <= '9')
                return c - '0';
        if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
        return -1;
}

Vec3 parse_hex_color(const std::string &token, bool &ok)
{
        ok = false;
        if (token.size() == 7 && token[0] == '#')
        {
                int r = 0;
                int g = 0;
                int b = 0;
                for (int i = 1; i < 7; ++i)
                {
                        int hv = hex_value(token[i]);
                        if (hv < 0)
                                return Vec3(0.0, 0.0, 0.0);
                        if (i < 3)
                                r = (r << 4) | hv;
                        else if (i < 5)
                                g = (g << 4) | hv;
                        else
                                b = (b << 4) | hv;
                }
                ok = true;
                return Vec3(r / 255.0, g / 255.0, b / 255.0);
        }
        if (token.size() == 13 && token[0] == '#')
        {
                int r = 0;
                int g = 0;
                int b = 0;
                for (int i = 1; i < 13; ++i)
                {
                        int hv = hex_value(token[i]);
                        if (hv < 0)
                                return Vec3(0.0, 0.0, 0.0);
                        if (i < 5)
                                r = (r << 4) | hv;
                        else if (i < 9)
                                g = (g << 4) | hv;
                        else
                                b = (b << 4) | hv;
                }
                ok = true;
                return Vec3(r / 65535.0, g / 65535.0, b / 65535.0);
        }
        return Vec3(0.0, 0.0, 0.0);
}

std::shared_ptr<Texture> load_xpm_texture(const std::string &path, std::string &error)
{
        std::ifstream in(path);
        if (!in)
        {
                error = "failed to open file";
                return nullptr;
        }

        std::vector<std::string> data;
        std::string line;
        while (std::getline(in, line))
        {
                auto first = line.find('"');
                if (first == std::string::npos)
                        continue;
                auto last = line.find('"', first + 1);
                if (last == std::string::npos)
                        continue;
                data.emplace_back(line.substr(first + 1, last - first - 1));
        }

        if (data.empty())
        {
                error = "empty texture";
                return nullptr;
        }

        std::istringstream header(data[0]);
        int width = 0;
        int height = 0;
        int colors = 0;
        int cpp = 0;
        if (!(header >> width >> height >> colors >> cpp))
        {
                error = "invalid xpm header";
                return nullptr;
        }
        if (width <= 0 || height <= 0 || colors <= 0 || cpp <= 0)
        {
                error = "invalid xpm dimensions";
                return nullptr;
        }
        if (static_cast<int>(data.size()) < 1 + colors + height)
        {
                error = "incomplete xpm data";
                return nullptr;
        }

        std::unordered_map<std::string, Vec3> palette;
        palette.reserve(static_cast<size_t>(colors));
        for (int i = 0; i < colors; ++i)
        {
                const std::string &entry = data[1 + i];
                if (static_cast<int>(entry.size()) < cpp)
                {
                        error = "invalid palette entry";
                        return nullptr;
                }
                std::string key = entry.substr(0, cpp);
                std::string rest = entry.substr(cpp);
                std::istringstream tokens(rest);
                std::string token;
                Vec3 color(0.0, 0.0, 0.0);
                bool found = false;
                while (tokens >> token)
                {
                        if (token == "c")
                        {
                                if (!(tokens >> token))
                                        break;
                                if (token == "None" || token == "none")
                                {
                                        color = Vec3(0.0, 0.0, 0.0);
                                        found = true;
                                        break;
                                }
                                bool ok = false;
                                Vec3 parsed = parse_hex_color(token, ok);
                                if (ok)
                                {
                                        color = parsed;
                                        found = true;
                                        break;
                                }
                        }
                }
                if (!found)
                {
                        auto hash = rest.find('#');
                        if (hash != std::string::npos)
                        {
                                size_t end = rest.find_first_of(" \t\r\n", hash);
                                std::string hex = rest.substr(hash, end - hash);
                                bool ok = false;
                                Vec3 parsed = parse_hex_color(hex, ok);
                                if (ok)
                                {
                                        color = parsed;
                                        found = true;
                                }
                        }
                }
                if (!found)
                        color = Vec3(0.0, 0.0, 0.0);
                palette[key] = color;
        }

        auto texture = std::make_shared<Texture>();
        texture->width_ = width;
        texture->height_ = height;
        texture->pixels_.assign(static_cast<size_t>(width * height), Vec3(0.0, 0.0, 0.0));
        for (int y = 0; y < height; ++y)
        {
                const std::string &row = data[1 + colors + y];
                if (static_cast<int>(row.size()) < width * cpp)
                {
                        error = "invalid pixel row";
                        return nullptr;
                }
                for (int x = 0; x < width; ++x)
                {
                        std::string key = row.substr(static_cast<size_t>(x * cpp), static_cast<size_t>(cpp));
                        auto it = palette.find(key);
                        Vec3 color = (it != palette.end()) ? it->second : Vec3(0.0, 0.0, 0.0);
                        texture->pixels_[static_cast<size_t>(y * width + x)] = color;
                }
        }

        return texture;
}

double wrap_coord(double value)
{
        double wrapped = std::fmod(value, 1.0);
        if (wrapped < 0.0)
                wrapped += 1.0;
        return wrapped;
}

} // namespace

Vec3 Texture::texel(int x, int y) const
{
        if (empty())
                return Vec3(1.0, 1.0, 1.0);
        int wx = width_ ? (x % width_) : 0;
        int wy = height_ ? (y % height_) : 0;
        if (wx < 0)
                wx += width_;
        if (wy < 0)
                wy += height_;
        return pixels_[static_cast<size_t>(wy * width_ + wx)];
}

Vec3 Texture::sample(double u, double v) const
{
        if (empty())
                return Vec3(1.0, 1.0, 1.0);
        double uw = wrap_coord(u);
        double vw = wrap_coord(v);
        double x = uw * static_cast<double>(width_);
        double y = vw * static_cast<double>(height_);
        int x0 = static_cast<int>(std::floor(x));
        double tx = x - static_cast<double>(x0);
        if (x0 >= width_)
        {
                x0 = width_ - 1;
                tx = 0.0;
        }
        int y0 = static_cast<int>(std::floor(y));
        double ty = y - static_cast<double>(y0);
        if (y0 >= height_)
        {
                y0 = height_ - 1;
                ty = 0.0;
        }
        int x1 = (x0 + 1) % width_;
        int y1 = (y0 + 1) % height_;
        Vec3 c00 = texel(x0, y0);
        Vec3 c10 = texel(x1, y0);
        Vec3 c01 = texel(x0, y1);
        Vec3 c11 = texel(x1, y1);
        Vec3 cx0 = c00 * (1.0 - tx) + c10 * tx;
        Vec3 cx1 = c01 * (1.0 - tx) + c11 * tx;
        return cx0 * (1.0 - ty) + cx1 * ty;
}

std::shared_ptr<Texture> load_texture(const std::string &path, std::string &error)
{
        std::filesystem::path fs_path(path);
        std::string ext = fs_path.extension().string();
        std::string lower_ext;
        lower_ext.resize(ext.size());
        std::transform(ext.begin(), ext.end(), lower_ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lower_ext == ".xpm")
        {
                return load_xpm_texture(path, error);
        }
        error = "unsupported texture format";
        return nullptr;
}
