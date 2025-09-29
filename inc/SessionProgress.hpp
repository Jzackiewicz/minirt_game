#pragma once

#include <string>

struct SessionProgress {
    bool has_progress = false;
    bool tutorial_mode = false;
    std::string base_directory;
    std::string next_scene_path;
    double total_score = 0.0;
    int completed_levels = 0;
    int total_levels = 0;
    std::string player_name;
};

const SessionProgress &get_session_progress();
void set_session_progress(const SessionProgress &progress);
void clear_session_progress();

