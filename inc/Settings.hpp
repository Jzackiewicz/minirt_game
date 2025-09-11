#pragma once
#include <string>

struct GameSettings {
    char quality;            // 'L', 'M', or 'H'
    float mouse_sensitivity; // multiplier applied to base sensitivity
    int width;               // window width
    int height;              // window height
};

extern GameSettings g_settings;

// When true, the renderer should save the current scene and reload
// with updated settings (used for quality changes).
extern bool g_reload_requested;
extern std::string g_reload_scene_path;

void load_settings(const std::string &filename = "settings.yaml");
void save_settings(const std::string &filename = "settings.yaml");
double get_mouse_sensitivity();

