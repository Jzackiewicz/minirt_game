#include "Settings.hpp"
#include <fstream>
#include <sstream>

Settings g_settings{'H', 1.0f, 1080, 720};

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos || end == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

bool load_settings(Settings &out) {
    Settings defaults{'H', 1.0f, 1080, 720};
    std::ifstream file("settings.yaml");
    if (!file) {
        out = defaults;
        g_settings = defaults;
        return false;
    }
    Settings s = defaults;
    std::string line;
    while (std::getline(file, line)) {
        std::size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        if (key == "quality") {
            if (value == "Low" || value == "low")
                s.quality = 'L';
            else if (value == "Medium" || value == "medium")
                s.quality = 'M';
            else
                s.quality = 'H';
        } else if (key == "mouse_sensitivity") {
            s.mouse_sensitivity = std::stof(value);
        } else if (key == "resolution") {
            std::size_t x = value.find('x');
            if (x != std::string::npos) {
                s.width = std::stoi(value.substr(0, x));
                s.height = std::stoi(value.substr(x + 1));
            }
        }
    }
    out = s;
    g_settings = s;
    return true;
}

void save_settings(const Settings &s) {
    std::ofstream file("settings.yaml");
    if (!file)
        return;
    std::string quality;
    switch (s.quality) {
    case 'L': quality = "Low"; break;
    case 'M': quality = "Medium"; break;
    default: quality = "High"; break;
    }
    file << "quality: " << quality << "\n";
    file << "mouse_sensitivity: " << s.mouse_sensitivity << "\n";
    file << "resolution: " << s.width << "x" << s.height << "\n";
}

