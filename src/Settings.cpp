#include "Settings.hpp"
#include <fstream>
#include <sstream>

Settings::Settings() : quality('H'), mouse_sensitivity(1.0), width(1080), height(720) {}

Settings Settings::load(const std::string &path) {
    Settings s;
    std::ifstream in(path);
    if (!in.is_open()) {
        return s;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, ':')) {
            std::string value;
            if (std::getline(iss, value)) {
                // trim spaces
                size_t start = value.find_first_not_of(" \t");
                if (start != std::string::npos)
                    value = value.substr(start);
                if (key == "quality") {
                    if (!value.empty())
                        s.quality = value[0];
                } else if (key == "mouse_sensitivity") {
                    double v = s.mouse_sensitivity;
                    std::istringstream(value) >> v;
                    s.mouse_sensitivity = v;
                } else if (key == "resolution") {
                    int w, h;
                    char x;
                    std::istringstream(value) >> w >> x >> h;
                    if (w > 0 && h > 0) {
                        s.width = w;
                        s.height = h;
                    }
                }
            }
        }
    }
    return s;
}

void Settings::save(const std::string &path) const {
    std::ofstream out(path);
    if (!out.is_open()) {
        return;
    }
    out << "quality: " << quality << "\n";
    out << "mouse_sensitivity: " << mouse_sensitivity << "\n";
    out << "resolution: " << width << 'x' << height << "\n";
}

Settings g_settings = Settings::load();
