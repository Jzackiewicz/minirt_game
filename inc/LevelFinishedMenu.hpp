#pragma once

#include "AMenu.hpp"

struct LevelFinishedMenuConfig {
    int current_level = 1;
    int total_levels = 1;
    double score = 0.0;
    double required_score = 0.0;
    double previous_total_score = 0.0;
};

class LevelFinishedMenu : public AMenu {
public:
    LevelFinishedMenu(const LevelFinishedMenuConfig &config);
    static ButtonAction show(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                             const LevelFinishedMenuConfig &config,
                             bool transparent = true);

private:
    LevelFinishedMenuConfig cfg;
    std::string name_input;
    bool input_active = true;

    ButtonAction run(SDL_Window *window, SDL_Renderer *renderer, int width, int height,
                     bool transparent);
};
