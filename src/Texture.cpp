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

std::string trim(const std::string &value)
{
        size_t start = 0;
        while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
                ++start;
        size_t end = value.size();
        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
                --end;
        return value.substr(start, end - start);
}

bool parse_hex_color(const std::string &token, Vec3 &out)
{
        if (token.size() != 7 || token[0] != '#')
                return false;
        auto hex_value = [](char c) -> int {
                if (c >= '0' && c <= '9')
                        return c - '0';
                if (c >= 'a' && c <= 'f')
                        return 10 + (c - 'a');
                if (c >= 'A' && c <= 'F')
                        return 10 + (c - 'A');
                return -1;
        };
        int components[3] = {0, 0, 0};
        for (int i = 0; i < 3; ++i)
        {
                int hi = hex_value(token[1 + i * 2]);
                int lo = hex_value(token[2 + i * 2]);
                if (hi < 0 || lo < 0)
                        return false;
                components[i] = hi * 16 + lo;
        }
        out = Vec3(components[0] / 255.0, components[1] / 255.0, components[2] / 255.0);
        return true;
}

bool parse_named_color(const std::string &token, Vec3 &out)
{
        static const std::unordered_map<std::string, Vec3> kNamed{
                {"black", Vec3(0.0, 0.0, 0.0)},
                {"white", Vec3(1.0, 1.0, 1.0)},
                {"red", Vec3(1.0, 0.0, 0.0)},
                {"green", Vec3(0.0, 1.0, 0.0)},
                {"blue", Vec3(0.0, 0.0, 1.0)},
                {"yellow", Vec3(1.0, 1.0, 0.0)},
                {"magenta", Vec3(1.0, 0.0, 1.0)},
                {"cyan", Vec3(0.0, 1.0, 1.0)},
                {"gray", Vec3(0.5, 0.5, 0.5)},
                {"grey", Vec3(0.5, 0.5, 0.5)},
        };
        auto it = kNamed.find(token);
        if (it == kNamed.end())
                return false;
        out = it->second;
        return true;
}

Vec3 parse_color_token(const std::string &token)
{
        Vec3 color(0.0, 0.0, 0.0);
        if (token == "None" || token == "none")
                return color;
        if (parse_hex_color(token, color))
                return color;
        if (parse_named_color(token, color))
                return color;
        return color;
}

bool extract_string_line(const std::string &line, std::string &out)
{
        size_t first = line.find('"');
        if (first == std::string::npos)
                return false;
        size_t last = line.find('"', first + 1);
        if (last == std::string::npos)
                return false;
        out = line.substr(first + 1, last - first - 1);
        return true;
}

} // namespace

Vec3 Texture::sample(double u, double v) const
{
        if (data.empty() || width <= 0 || height <= 0)
                return Vec3(0.0, 0.0, 0.0);
        if (!std::isfinite(u) || !std::isfinite(v))
                return Vec3(0.0, 0.0, 0.0);
        double u_wrapped = u - std::floor(u);
        double v_wrapped = v - std::floor(v);
        if (u_wrapped < 0.0)
                u_wrapped += 1.0;
        if (v_wrapped < 0.0)
                v_wrapped += 1.0;
        int x = static_cast<int>(u_wrapped * static_cast<double>(width));
        int y = static_cast<int>((1.0 - v_wrapped) * static_cast<double>(height));
        if (x < 0)
                x = 0;
        if (x >= width)
                x = width - 1;
        if (y < 0)
                y = 0;
        if (y >= height)
                y = height - 1;
        return data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
}

bool load_xpm_texture(const std::string &path, Texture &out, std::string &error_message)
{
        std::ifstream in(path);
        if (!in)
        {
                error_message = "cannot open file";
                return false;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line))
        {
                std::string extracted;
                if (!extract_string_line(line, extracted))
                        continue;
                lines.push_back(extracted);
        }

        if (lines.empty())
        {
                error_message = "missing XPM data";
                return false;
        }

        std::istringstream header_stream(lines[0]);
        int width = 0;
        int height = 0;
        int num_colors = 0;
        int chars_per_pixel = 0;
        if (!(header_stream >> width >> height >> num_colors >> chars_per_pixel))
        {
                error_message = "invalid header";
                return false;
        }
        if (width <= 0 || height <= 0 || num_colors <= 0 || chars_per_pixel <= 0)
        {
                error_message = "invalid dimensions";
                return false;
        }
        if (static_cast<int>(lines.size()) < 1 + num_colors + height)
        {
                error_message = "incomplete XPM data";
                return false;
        }

        std::unordered_map<std::string, Vec3> palette;
        palette.reserve(static_cast<size_t>(num_colors));
        for (int i = 0; i < num_colors; ++i)
        {
                const std::string &entry = lines[1 + i];
                if (static_cast<int>(entry.size()) < chars_per_pixel)
                {
                        error_message = "invalid color entry";
                        return false;
                }
                std::string key = entry.substr(0, static_cast<size_t>(chars_per_pixel));
                std::string rest = trim(entry.substr(static_cast<size_t>(chars_per_pixel)));
                std::istringstream rest_stream(rest);
                std::string token;
                bool color_found = false;
                while (rest_stream >> token)
                {
                        if (token == "c" || token == "c#")
                        {
                                std::string value;
                                if (!(rest_stream >> value))
                                {
                                        error_message = "missing color value";
                                        return false;
                                }
                                palette[key] = parse_color_token(value);
                                color_found = true;
                                break;
                        }
                        if (token.size() == 1 && token[0] == 'c')
                        {
                                std::string value;
                                if (!(rest_stream >> value))
                                {
                                        error_message = "missing color value";
                                        return false;
                                }
                                palette[key] = parse_color_token(value);
                                color_found = true;
                                break;
                        }
                }
                if (!color_found)
                {
                        // Some XPM variants have the color value immediately after 'c'
                        size_t pos = rest.find('c');
                        if (pos != std::string::npos)
                        {
                                std::string value = trim(rest.substr(pos + 1));
                                std::istringstream value_stream(value);
                                std::string first;
                                if (value_stream >> first)
                                {
                                        palette[key] = parse_color_token(first);
                                        color_found = true;
                                }
                        }
                }
                if (!color_found)
                {
                        error_message = "color entry missing 'c' definition";
                        return false;
                }
        }

        out.width = width;
        out.height = height;
        out.data.assign(static_cast<size_t>(width * height), Vec3(0.0, 0.0, 0.0));

        for (int row = 0; row < height; ++row)
        {
                const std::string &data_line = lines[1 + num_colors + row];
                if (static_cast<int>(data_line.size()) < width * chars_per_pixel)
                {
                        error_message = "pixel row too short";
                        return false;
                }
                for (int col = 0; col < width; ++col)
                {
                        size_t pos = static_cast<size_t>(col * chars_per_pixel);
                        std::string key = data_line.substr(pos, static_cast<size_t>(chars_per_pixel));
                        auto it = palette.find(key);
                        if (it == palette.end())
                        {
                                error_message = "pixel references undefined color";
                                return false;
                        }
                        out.data[static_cast<size_t>(row) * static_cast<size_t>(width) + static_cast<size_t>(col)] =
                                it->second;
                }
        }

        return true;
}
