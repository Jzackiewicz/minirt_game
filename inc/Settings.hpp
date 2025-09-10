#pragma once
#include <string>

struct GameSettings {
    char quality = 'H';
    double mouse_sensitivity = 1.0; // multiplier
    int width = 1080;
    int height = 720;
};

extern GameSettings g_settings;

bool load_settings(const std::string &path = "settings.yaml");
bool save_settings(const std::string &path = "settings.yaml");
