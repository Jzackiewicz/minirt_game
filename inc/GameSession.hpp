#pragma once

#include <string>

struct GameSession {
    bool has_progress = false;
    bool tutorial_mode = false;
    std::string next_scene_path;
    double cumulative_score = 0.0;
    int completed_levels = 0;
    std::string player_name;
};

