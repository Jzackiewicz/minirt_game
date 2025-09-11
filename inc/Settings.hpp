#pragma once
#include <string>

// Structure holding configurable settings loaded from/saved to YAML.
struct Settings {
    char quality;           // 'L', 'M', 'H'
    float mouse_sensitivity; // multiplier for MOUSE_SENSITIVITY
    int width;              // window width
    int height;             // window height
};

// Global settings instance used throughout the program.
extern Settings g_settings;

// Load settings from a YAML file. If the file does not exist, defaults are used.
bool load_settings(Settings &out);

// Save settings to a YAML file.
void save_settings(const Settings &s);

