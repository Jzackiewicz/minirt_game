#pragma once

#include "AMenu.hpp"
#include <string>

struct LevelFinishedInfo {
    int level_number = 1;
    int total_levels = 1;
    double current_score = 0.0;
    double required_score = 0.0;
    double accumulated_score = 0.0;
    bool has_next_level = false;
};

class LevelFinishedMenu : public AMenu {
public:
    LevelFinishedMenu();
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                     const LevelFinishedInfo &info, bool transparent = true);
    static ButtonAction show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                             const LevelFinishedInfo &info, bool transparent = true);
};
