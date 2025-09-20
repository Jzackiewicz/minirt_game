#include "Texture.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
std::string trim(const std::string &s)
{
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
                ++start;
        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
                --end;
        return s.substr(start, end - start);
}

bool parse_hex_color(const std::string &token, Vec3 &out)
{
        if (token.empty())
                return false;
        if (token[0] != '#')
                return false;
        if (token.size() == 7)
        {
                auto hex_to_int = [](char c) -> int {
                        if (c >= '0' && c <= '9')
                                return c - '0';
                        if (c >= 'a' && c <= 'f')
                                return 10 + (c - 'a');
                        if (c >= 'A' && c <= 'F')
                                return 10 + (c - 'A');
                        return 0;
                };
                int r = hex_to_int(token[1]) * 16 + hex_to_int(token[2]);
                int g = hex_to_int(token[3]) * 16 + hex_to_int(token[4]);
                int b = hex_to_int(token[5]) * 16 + hex_to_int(token[6]);
                out = Vec3(r / 255.0, g / 255.0, b / 255.0);
                return true;
        }
        if (token.size() == 4)
        {
                auto hex_to_int = [](char c) -> int {
                        if (c >= '0' && c <= '9')
                                return c - '0';
                        if (c >= 'a' && c <= 'f')
                                return 10 + (c - 'a');
                        if (c >= 'A' && c <= 'F')
                                return 10 + (c - 'A');
                        return 0;
                };
                int r = hex_to_int(token[1]);
                int g = hex_to_int(token[2]);
                int b = hex_to_int(token[3]);
                out = Vec3(r / 15.0, g / 15.0, b / 15.0);
                return true;
        }
        return false;
}

bool parse_rgb_color(const std::string &token, Vec3 &out)
{
        if (token.size() < 5)
                return false;
        if (token.rfind("rgb:", 0) != 0)
                return false;
        std::string rest = token.substr(4);
        std::replace(rest.begin(), rest.end(), '/', ' ');
        std::istringstream iss(rest);
        int r = 0;
        int g = 0;
        int b = 0;
        if (!(iss >> std::hex >> r >> g >> b))
                return false;
        out = Vec3(r / 255.0, g / 255.0, b / 255.0);
        return true;
}

Vec3 parse_color_token(const std::vector<std::string> &tokens)
{
        for (size_t i = 0; i < tokens.size(); ++i)
        {
                std::string lower = tokens[i];
                std::transform(lower.begin(), lower.end(), lower.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (lower == "c" && i + 1 < tokens.size())
                {
                        Vec3 out;
                        if (parse_hex_color(tokens[i + 1], out))
                                return out;
                        if (parse_rgb_color(tokens[i + 1], out))
                                return out;
                        std::string next_lower = tokens[i + 1];
                        std::transform(next_lower.begin(), next_lower.end(), next_lower.begin(),
                                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        if (next_lower == "none")
                                return Vec3(0.0, 0.0, 0.0);
                }
        }
        return Vec3(0.0, 0.0, 0.0);
}

std::vector<std::string> split_tokens(const std::string &str)
{
        std::vector<std::string> tokens;
        std::istringstream iss(str);
        std::string token;
        while (iss >> token)
                tokens.push_back(token);
        return tokens;
}

} // namespace

bool Texture::load_from_file(const std::string &path)
{
        width = 0;
        height = 0;
        pixels.clear();
        std::ifstream in(path);
        if (!in)
                return false;
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line))
        {
                size_t start = line.find('"');
                if (start == std::string::npos)
                        continue;
                size_t end = line.find_last_of('"');
                if (end == std::string::npos || end <= start)
                        continue;
                std::string inner = line.substr(start + 1, end - start - 1);
                lines.push_back(inner);
        }
        if (lines.empty())
                return false;
        std::istringstream header(lines[0]);
        int num_colors = 0;
        int cpp = 0;
        if (!(header >> width >> height >> num_colors >> cpp))
                return false;
        if (width <= 0 || height <= 0 || num_colors <= 0 || cpp <= 0)
                return false;
        if (static_cast<int>(lines.size()) < 1 + num_colors + height)
                return false;
        std::unordered_map<std::string, Vec3> color_table;
        for (int i = 0; i < num_colors; ++i)
        {
                std::string entry = lines[1 + i];
                if (static_cast<int>(entry.size()) < cpp)
                        return false;
                std::string key = entry.substr(0, cpp);
                std::string rest = entry.substr(cpp);
                rest = trim(rest);
                std::vector<std::string> tokens = split_tokens(rest);
                Vec3 color = parse_color_token(tokens);
                color_table[key] = color;
        }
        pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
        for (int y = 0; y < height; ++y)
        {
                std::string row = lines[1 + num_colors + y];
                if (static_cast<int>(row.size()) < width * cpp)
                        return false;
                for (int x = 0; x < width; ++x)
                {
                        std::string key = row.substr(x * cpp, cpp);
                        auto it = color_table.find(key);
                        Vec3 color = Vec3(0.0, 0.0, 0.0);
                        if (it != color_table.end())
                                color = it->second;
                        pixels[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = color;
                }
        }
        return true;
}

Vec3 Texture::get_pixel(int x, int y) const
{
        x = (x % width + width) % width;
        y = (y % height + height) % height;
        return pixels[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
}

Vec3 Texture::sample(double u, double v) const
{
        if (!is_valid())
                return Vec3(0.0, 0.0, 0.0);
        double u_wrapped = u - std::floor(u);
        double v_wrapped = v - std::floor(v);
        if (u_wrapped < 0.0)
                u_wrapped += 1.0;
        if (v_wrapped < 0.0)
                v_wrapped += 1.0;
        double x = u_wrapped * (width - 1);
        double y = (1.0 - v_wrapped) * (height - 1);
        int x0 = static_cast<int>(std::floor(x));
        int y0 = static_cast<int>(std::floor(y));
        int x1 = x0 + 1;
        int y1 = y0 + 1;
        double tx = x - x0;
        double ty = y - y0;
        Vec3 c00 = get_pixel(x0, y0);
        Vec3 c10 = get_pixel(x1, y0);
        Vec3 c01 = get_pixel(x0, y1);
        Vec3 c11 = get_pixel(x1, y1);
        Vec3 c0 = c00 * (1.0 - tx) + c10 * tx;
        Vec3 c1 = c01 * (1.0 - tx) + c11 * tx;
        return c0 * (1.0 - ty) + c1 * ty;
}

