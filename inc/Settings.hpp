#pragma once
#include <string>

struct Settings {
    char quality;           // 'L', 'M', or 'H'
    double mouse_sensitivity; // multiplier applied to MOUSE_SENSITIVITY
    int width;
    int height;

    Settings();
    static Settings load(const std::string &path = "settings.yaml");
    void save(const std::string &path = "settings.yaml") const;
};

extern Settings g_settings;
