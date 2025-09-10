#include "Settings.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

GameSettings g_settings; // defined global

static std::string trim(const std::string &s) {
    std::string::size_type start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    std::string::size_type end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

bool load_settings(const std::string &path) {
    g_settings = GameSettings(); // reset to defaults
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        if (key == "quality" && !value.empty()) {
            g_settings.quality = value[0];
        } else if (key == "mouse_sensitivity") {
            g_settings.mouse_sensitivity = std::stod(value);
        } else if (key == "resolution") {
            auto x = value.find('x');
            if (x != std::string::npos) {
                g_settings.width = std::stoi(value.substr(0, x));
                g_settings.height = std::stoi(value.substr(x + 1));
            }
        }
    }
    return true;
}

bool save_settings(const std::string &path) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    out << "quality: " << g_settings.quality << "\n";
    out << "mouse_sensitivity: " << g_settings.mouse_sensitivity << "\n";
    out << "resolution: " << g_settings.width << "x" << g_settings.height << "\n";
    return true;
}
