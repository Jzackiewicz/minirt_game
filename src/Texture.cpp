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

double wrap_coord(double value)
{
        value -= std::floor(value);
        if (value < 0.0)
                value += 1.0;
        return value;
}

bool parse_hex(const std::string &str, Vec3 &out)
{
        if (str.size() == 7 && str[0] == '#')
        {
                int r = std::stoi(str.substr(1, 2), nullptr, 16);
                int g = std::stoi(str.substr(3, 2), nullptr, 16);
                int b = std::stoi(str.substr(5, 2), nullptr, 16);
                out = Vec3(r / 255.0, g / 255.0, b / 255.0);
                return true;
        }
        if (str.size() == 4 && str[0] == '#')
        {
                int r = std::stoi(str.substr(1, 1), nullptr, 16);
                int g = std::stoi(str.substr(2, 1), nullptr, 16);
                int b = std::stoi(str.substr(3, 1), nullptr, 16);
                out = Vec3((r * 17) / 255.0, (g * 17) / 255.0, (b * 17) / 255.0);
                return true;
        }
        return false;
}

Vec3 parse_named_color(const std::string &name, bool &ok)
{
        static const std::unordered_map<std::string, Vec3> colors = {
                {"black", Vec3(0.0, 0.0, 0.0)},     {"white", Vec3(1.0, 1.0, 1.0)},
                {"red", Vec3(1.0, 0.0, 0.0)},       {"green", Vec3(0.0, 1.0, 0.0)},
                {"blue", Vec3(0.0, 0.0, 1.0)},      {"yellow", Vec3(1.0, 1.0, 0.0)},
                {"magenta", Vec3(1.0, 0.0, 1.0)},   {"cyan", Vec3(0.0, 1.0, 1.0)},
                {"grey", Vec3(0.5, 0.5, 0.5)},      {"gray", Vec3(0.5, 0.5, 0.5)},
                {"orange", Vec3(1.0, 0.5, 0.0)},    {"purple", Vec3(0.5, 0.0, 0.5)},
        };
        auto it = colors.find(name);
        if (it != colors.end())
        {
                ok = true;
                return it->second;
        }
        ok = false;
        return Vec3(0.0, 0.0, 0.0);
}

} // namespace

Vec3 Texture::sample(double u, double v) const
{
        if (!valid())
                return Vec3(1.0, 1.0, 1.0);
        double uu = wrap_coord(u);
        double vv = wrap_coord(v);
        int ix = static_cast<int>(std::floor(uu * width));
        int iy = static_cast<int>(std::floor((1.0 - vv) * height));
        if (ix < 0)
                ix = 0;
        if (iy < 0)
                iy = 0;
        if (ix >= width)
                ix = width - 1;
        if (iy >= height)
                iy = height - 1;
        return pixels[static_cast<size_t>(iy) * static_cast<size_t>(width) + static_cast<size_t>(ix)];
}

bool load_xpm_texture(const std::string &path, Texture &out, std::string &error)
{
        std::ifstream file(path);
        if (!file)
        {
                error = "unable to open file";
                return false;
        }
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line))
        {
                size_t first = line.find('"');
                if (first == std::string::npos)
                        continue;
                size_t last = line.find_last_of('"');
                if (last <= first)
                        continue;
                lines.emplace_back(line.substr(first + 1, last - first - 1));
        }
        if (lines.empty())
        {
                error = "missing XPM header";
                return false;
        }
        std::istringstream header(lines[0]);
        int width = 0;
        int height = 0;
        int colors = 0;
        int cpp = 0;
        if (!(header >> width >> height >> colors >> cpp))
        {
                error = "invalid XPM header";
                return false;
        }
        if (width <= 0 || height <= 0 || colors <= 0 || cpp <= 0)
        {
                error = "XPM header values out of range";
                return false;
        }
        if (static_cast<int>(lines.size()) < 1 + colors + height)
        {
                error = "XPM data truncated";
                return false;
        }
        std::unordered_map<std::string, Vec3> palette;
        palette.reserve(static_cast<size_t>(colors));
        for (int i = 0; i < colors; ++i)
        {
                const std::string &entry = lines[1 + i];
                if (static_cast<int>(entry.size()) < cpp)
                {
                        error = "invalid color entry";
                        return false;
                }
                std::string key = entry.substr(0, cpp);
                std::string rest = entry.substr(cpp);
                std::istringstream iss(rest);
                std::string token;
                Vec3 value(0.0, 0.0, 0.0);
                bool have_color = false;
                while (iss >> token)
                {
                        if (token == "c" || token == "C")
                        {
                                if (!(iss >> token))
                                        break;
                                std::string lower;
                                lower.reserve(token.size());
                                for (char ch : token)
                                        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
                                if (lower == "none")
                                {
                                        value = Vec3(0.0, 0.0, 0.0);
                                        have_color = true;
                                }
                                else if (parse_hex(token, value))
                                {
                                        have_color = true;
                                }
                                else
                                {
                                        bool ok = false;
                                        value = parse_named_color(lower, ok);
                                        have_color = ok;
                                }
                                break;
                        }
                }
                if (!have_color)
                {
                        error = "unsupported color specification";
                        return false;
                }
                palette[key] = value;
        }
        out.width = width;
        out.height = height;
        out.pixels.assign(static_cast<size_t>(width * height), Vec3(0.0, 0.0, 0.0));
        for (int y = 0; y < height; ++y)
        {
                const std::string &row = lines[1 + colors + y];
                if (static_cast<int>(row.size()) < cpp * width)
                {
                        error = "invalid pixel row";
                        return false;
                }
                for (int x = 0; x < width; ++x)
                {
                        std::string code = row.substr(x * cpp, cpp);
                        auto it = palette.find(code);
                        if (it == palette.end())
                        {
                                error = "undefined color code";
                                return false;
                        }
                        out.pixels[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = it->second;
                }
        }
        error.clear();
        return true;
}
