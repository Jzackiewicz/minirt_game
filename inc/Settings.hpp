#pragma once
#include <string>

struct Settings {
    int width = 1080;
    int height = 720;
    char quality = 'H';
    double mouse_sensitivity = 1.0; // multiplier
};

Settings &get_settings();
bool load_settings(const std::string &path);
bool save_settings(const std::string &path);
