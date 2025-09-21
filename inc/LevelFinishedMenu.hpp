#pragma once

#include "AMenu.hpp"
#include <string>

struct LevelFinishedStats {
    int completed_levels = 0;
    int total_levels = 0;
    double current_score = 0.0;
    double required_score = 0.0;
    double total_score = 0.0;
    bool has_next_level = false;
};

class LevelFinishedMenu : public AMenu {
public:
    LevelFinishedMenu(const LevelFinishedStats &stats, std::string &player_name);
    static ButtonAction show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                             const LevelFinishedStats &stats, std::string &player_name,
                             bool transparent = true);

private:
    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                     bool transparent);

    LevelFinishedStats stats_;
    std::string &player_name_;
};
