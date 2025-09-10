#include "Settings.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

static Settings g_settings; // initialized with defaults

Settings &get_settings() { return g_settings; }

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

bool load_settings(const std::string &path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, ':')) {
            std::string value;
            if (std::getline(iss, value)) {
                key = trim(key);
                value = trim(value);
                if (key == "quality" && !value.empty()) {
                    char q = value[0];
                    if (q=='L'||q=='M'||q=='H'||q=='l'||q=='m'||q=='h')
                        g_settings.quality = toupper(q);
                } else if (key == "mouse_sensitivity") {
                    g_settings.mouse_sensitivity = std::stod(value);
                } else if (key == "width") {
                    g_settings.width = std::stoi(value);
                } else if (key == "height") {
                    g_settings.height = std::stoi(value);
                }
            }
        }
    }
    return true;
}

bool save_settings(const std::string &path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "quality: " << g_settings.quality << "\n";
    out << "mouse_sensitivity: " << g_settings.mouse_sensitivity << "\n";
    out << "width: " << g_settings.width << "\n";
    out << "height: " << g_settings.height << "\n";
    return true;
}
