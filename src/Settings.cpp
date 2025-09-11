#include "Settings.hpp"
#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

GameSettings g_settings{'H', 1.0f, 1080, 720};
bool g_in_game = false;
bool g_reload_requested = false;
std::string g_reload_scene_path;

static std::string trim(const std::string &s) {
    const char *ws = " \t\n\r";
    size_t start = s.find_first_not_of(ws);
    size_t end = s.find_last_not_of(ws);
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

void load_settings(const std::string &filename) {
    std::ifstream file(filename);
    if (!file)
        return; // keep defaults
    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos)
            continue;
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        if (key == "quality") {
            if (value == "Low" || value == "LOW" || value == "low")
                g_settings.quality = 'L';
            else if (value == "Medium" || value == "MEDIUM" || value == "medium")
                g_settings.quality = 'M';
            else
                g_settings.quality = 'H';
        } else if (key == "mouse_sensitivity") {
            g_settings.mouse_sensitivity = std::strtof(value.c_str(), nullptr);
        } else if (key == "resolution") {
            auto x = value.find('x');
            if (x != std::string::npos) {
                g_settings.width = std::strtol(value.substr(0, x).c_str(), nullptr, 10);
                g_settings.height =
                    std::strtol(value.substr(x + 1).c_str(), nullptr, 10);
            }
        }
    }
}

void save_settings(const std::string &filename) {
    std::ofstream file(filename);
    if (!file)
        return;
    file << "quality: ";
    if (g_settings.quality == 'L')
        file << "Low";
    else if (g_settings.quality == 'M')
        file << "Medium";
    else
        file << "High";
    file << '\n';
    file << std::fixed << std::setprecision(1);
    file << "mouse_sensitivity: " << g_settings.mouse_sensitivity << '\n';
    file << "resolution: " << g_settings.width << 'x' << g_settings.height << '\n';
}

double get_mouse_sensitivity() {
    return MOUSE_SENSITIVITY * static_cast<double>(g_settings.mouse_sensitivity);
}

